// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2006      by James Haley
// Copyright (C) 2006-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_polyobj.c
/// \brief Movable segs like in Hexen, but more flexible
///        due to application of dynamic binary space partitioning theory.

// haleyjd: temporary define

#include "z_zone.h"

#include "doomstat.h"
#include "g_game.h"
#include "m_bbox.h"
#include "m_queue.h"
#include "p_maputl.h"
#include "p_setup.h"
#include "p_tick.h"
#include "p_local.h"
#include "p_polyobj.h"
#include "r_main.h"
#include "r_state.h"
#include "r_defs.h"

/*
   Theory behind Polyobjects:

      "The BSP tree hidden surface removal algorithm can easily be
      extended to allow for dynamic objects. For each frame, start with
      a BSP tree containing all the static objects in the scene, and
      reinsert the dynamic objects. While this is straightforward to
      implement, it can involve substantial computation.

      "If a dynamic object is separated from each static object by a
      plane, the dynamic object can be represented as a single point
      regardless of its complexity. This can dramatically reduce the
      computation per frame because only one node per dynamic object is
      inserted into the BSP tree. Compare that to one node for every
      polygon in the object, and the reason for the savings is obvious.
      During tree traversal, each point is expanded into the original
      object...

      "Inserting a point into the BSP tree is very cheap, because there
      is only one front/back test at each node. Points are never split,
      which explains the requirement of separation by a plane. The
      dynamic object will always be drawn completely in front of the
      static objects behind it.

      "...a different front/back test is necessary, because a point
      doesn't partition three dimesnional (sic) space. The correct
      front/back test is to simply compare distances to the eye. Once
      computed, this distance can be cached at the node until the frame
      is drawn."

   From http://www.faqs.org/faqs/graphics/bsptree-faq/ (The BSP FAQ)

   While Hexen had polyobjects, it put severe and artificial limits upon
   them by keeping them attached to one subsector, and allowing only one
   per subsector. Neither is necessary, and removing those limitations
   results in the free-moving polyobjects implemented here. The only
   true - and unavoidable - restriction is that polyobjects should never
   overlap with each other or with static walls.

   The reason that multiple polyobjects per subsector is viable is that
   with the above assumption that the objects will not overlap, if the
   center point of one polyobject is closer to the viewer than the center
   point of another, then the entire polyobject is closer to the viewer.
   In this way it is possible to impose an order on polyobjects within a
   subsector, as well as allowing the BSP tree to impose its natural
   ordering on polyobjects amongst all subsectors.
*/

//
// Defines
//

#define BYTEANGLEMUL     (ANGLE_11hh/8)

//
// Globals
//

// The Polyobjects
polyobj_t *PolyObjects;
INT32 numPolyObjects;

// Polyobject Blockmap -- initialized in P_LoadBlockMap
polymaplink_t **polyblocklinks;


//
// Static Data
//

// Polyobject Blockmap
static polymaplink_t *bmap_freelist; // free list of blockmap links


//
// Static Functions
//

FUNCINLINE static ATTRINLINE void Polyobj_bboxAdd(fixed_t *bbox, vertex_t *add)
{
	bbox[BOXTOP]    += add->y;
	bbox[BOXBOTTOM] += add->y;
	bbox[BOXLEFT]   += add->x;
	bbox[BOXRIGHT]  += add->x;
}

FUNCINLINE static ATTRINLINE void Polyobj_bboxSub(fixed_t *bbox, vertex_t *sub)
{
	bbox[BOXTOP]    -= sub->y;
	bbox[BOXBOTTOM] -= sub->y;
	bbox[BOXLEFT]   -= sub->x;
	bbox[BOXRIGHT]  -= sub->x;
}

FUNCINLINE static ATTRINLINE void Polyobj_vecAdd(vertex_t *dst, vertex_t *add)
{
	dst->x += add->x;
	dst->y += add->y;
}

FUNCINLINE static ATTRINLINE void Polyobj_vecSub(vertex_t *dst, vertex_t *sub)
{
	dst->x -= sub->x;
	dst->y -= sub->y;
}

FUNCINLINE static ATTRINLINE void Polyobj_vecSub2(vertex_t *dst, vertex_t *v1, vertex_t *v2)
{
	dst->x = v1->x - v2->x;
	dst->y = v1->y - v2->y;
}

boolean P_PointInsidePolyobj(polyobj_t *po, fixed_t x, fixed_t y)
{
	size_t i;

	for (i = 0; i < po->numLines; i++)
	{
		if (P_PointOnLineSide(x, y, po->lines[i]) == 0)
			return false;
	}

	return true;
}

boolean P_MobjTouchingPolyobj(polyobj_t *po, mobj_t *mo)
{
	fixed_t mbbox[4];
	size_t i;

	mbbox[BOXTOP] = mo->y + mo->radius;
	mbbox[BOXBOTTOM] = mo->y - mo->radius;
	mbbox[BOXRIGHT] = mo->x + mo->radius;
	mbbox[BOXLEFT] = mo->x - mo->radius;

	for (i = 0; i < po->numLines; i++)
	{
		if (P_BoxOnLineSide(mbbox, po->lines[i]) == -1)
			return true;
	}

	return false;
}

boolean P_MobjInsidePolyobj(polyobj_t *po, mobj_t *mo)
{
	fixed_t mbbox[4];
	size_t i;

	mbbox[BOXTOP] = mo->y + mo->radius;
	mbbox[BOXBOTTOM] = mo->y - mo->radius;
	mbbox[BOXRIGHT] = mo->x + mo->radius;
	mbbox[BOXLEFT] = mo->x - mo->radius;

	for (i = 0; i < po->numLines; i++)
	{
		if (P_BoxOnLineSide(mbbox, po->lines[i]) == 0)
			return false;
	}

	return true;
}

boolean P_BBoxInsidePolyobj(polyobj_t *po, fixed_t *bbox)
{
	size_t i;

	for (i = 0; i < po->numLines; i++)
	{
		if (P_BoxOnLineSide(bbox, po->lines[i]) == 0)
			return false;
	}

	return true;
}

// Finds the 'polyobject settings' linedef for a polyobject
// the polyobject's id should be set as its tag
static void Polyobj_GetInfo(polyobj_t *po)
{
	INT32 i = P_FindSpecialLineFromTag(POLYINFO_SPECIALNUM, po->id, -1);

	po->flags = POF_SOLID|POF_TESTHEIGHT|POF_RENDERSIDES;

	if (i == -1)
		return; // no extra settings to apply, let's leave it

	po->parent = lines[i].frontsector->special;
	if (po->parent == po->id) // do not allow a self-reference
		po->parent = -1;

	po->translucency = (lines[i].flags & ML_DONTPEGTOP)
						? (sides[lines[i].sidenum[0]].textureoffset>>FRACBITS)
						: ((lines[i].frontsector->floorheight>>FRACBITS) / 100);

	po->translucency = max(min(po->translucency, NUMTRANSMAPS), 0);

	if (lines[i].flags & ML_EFFECT1)
		po->flags |= POF_ONESIDE;

	if (lines[i].flags & ML_EFFECT2)
		po->flags &= ~POF_SOLID;

	if (lines[i].flags & ML_EFFECT3)
		po->flags |= POF_PUSHABLESTOP;

	if (lines[i].flags & ML_EFFECT4)
		po->flags |= POF_RENDERPLANES;

	/*if (lines[i].flags & ML_EFFECT5)
		po->flags &= ~POF_CLIPPLANES;*/

	if (lines[i].flags & ML_EFFECT6)
		po->flags |= POF_SPLAT;

	if (lines[i].flags & ML_NOCLIMB) // Has a linedef executor
		po->flags |= POF_LDEXEC;
}

// Reallocating array maintenance

// Adds a vertex to a polyobject's reallocating vertex arrays, provided
// that such a vertex isn't already in the array. Each vertex must only
// be translated once during polyobject movement. Keeping track of them
// this way results in much more clear and efficient code than what
// Hexen used.
static void Polyobj_addVertex(polyobj_t *po, vertex_t *v)
{
	size_t i;

	// First: search the existing vertex pointers for a match. If one is found,
	// do not add this vertex again.
	for (i = 0; i < po->numVertices; ++i)
	{
		if (po->vertices[i] == v)
			return;
	}

	// add the vertex to all arrays (translation for origVerts is done later)
	if (po->numVertices >= po->numVerticesAlloc)
	{
		po->numVerticesAlloc = po->numVerticesAlloc ? po->numVerticesAlloc * 2 : 4;
		po->vertices =
			(vertex_t **)Z_Realloc(po->vertices,
			                       po->numVerticesAlloc * sizeof(vertex_t *),
			                       PU_LEVEL, NULL);
		po->origVerts =
			(vertex_t *)Z_Realloc(po->origVerts,
			                      po->numVerticesAlloc * sizeof(vertex_t),
			                      PU_LEVEL, NULL);

		po->tmpVerts =
			(vertex_t *)Z_Realloc(po->tmpVerts,
			                      po->numVerticesAlloc * sizeof(vertex_t),
			                      PU_LEVEL, NULL);
	}
	po->vertices[po->numVertices] = v;
	po->origVerts[po->numVertices] = *v;
	po->numVertices++;
}

// Adds a linedef to a polyobject's reallocating linedefs array, provided
// that such a linedef isn't already in the array. Each linedef must only
// be adjusted once during polyobject movement. Keeping track of them
// this way provides the same benefits as for vertices.
static void Polyobj_addLine(polyobj_t *po, line_t *l)
{
	size_t i;

	// First: search the existing line pointers for a match. If one is found,
	// do not add this line again.
	for (i = 0; i < po->numLines; ++i)
	{
		if (po->lines[i] == l)
			return;
	}

	// add the line to the array
	if (po->numLines >= po->numLinesAlloc)
	{
		po->numLinesAlloc = po->numLinesAlloc ? po->numLinesAlloc * 2 : 4;
		po->lines = (line_t **)Z_Realloc(po->lines,
										po->numLinesAlloc * sizeof(line_t *),
										PU_LEVEL, NULL);
	}
	l->polyobj = po;
	po->lines[po->numLines++] = l;
}

// Adds a single seg to a polyobject's reallocating seg pointer array.
// Most polyobjects will have between 4 and 16 segs, so the array size
// begins much smaller than usual. Calls Polyobj_addVertex and Polyobj_addLine
// to add those respective structures for this seg, as well.
static void Polyobj_addSeg(polyobj_t *po, seg_t *seg)
{
	if (po->segCount >= po->numSegsAlloc)
	{
		po->numSegsAlloc = po->numSegsAlloc ? po->numSegsAlloc * 2 : 4;
		po->segs = (seg_t **)Z_Realloc(po->segs,
										po->numSegsAlloc * sizeof(seg_t *),
										PU_LEVEL, NULL);
	}

	seg->polyseg = po;

	po->segs[po->segCount++] = seg;

	// possibly add the lines and vertices for this seg. It may be technically
	// unnecessary to add the v2 vertex of segs, but this makes sure that even
	// erroneously open "explicit" segs will have both vertices added and will
	// reduce problems.
	Polyobj_addVertex(po, seg->v1);
	Polyobj_addVertex(po, seg->v2);
	Polyobj_addLine(po, seg->linedef);
}

// Seg-finding functions

// This method adds segs to a polyobject by following segs from vertex to
// vertex.  The process stops when the original starting point is reached
// or if a particular search ends unexpectedly (ie, the polyobject is not
// closed).
static void Polyobj_findSegs(polyobj_t *po, seg_t *seg)
{
	fixed_t startx, starty;
	size_t i;
	size_t s;

	Polyobj_addSeg(po, seg);

	if (!(po->flags & POF_ONESIDE))
	{
		// Find backfacings
		for (s = 0;  s < numsegs; s++)
		{
			size_t r;

			if (segs[s].glseg)
				continue;

			if (segs[s].linedef != seg->linedef)
				continue;

			if (segs[s].side != 1)
				continue;

			for (r = 0; r < po->segCount; r++)
			{
				if (po->segs[r] == &segs[s])
					break;
			}

			if (r != po->segCount)
				continue;

			segs[s].dontrenderme = true;

			Polyobj_addSeg(po, &segs[s]);
		}
	}

	// on first seg, save the initial vertex
	startx = seg->v1->x;
	starty = seg->v1->y;

	// use goto instead of recursion for maximum efficiency - thanks to lament
newseg:

	// terminal case: we have reached a seg where v2 is the same as v1 of the
	// initial seg
	if (seg->v2->x == startx && seg->v2->y == starty)
		return;

	// search the segs for one whose starting vertex is equal to the current
	// seg's ending vertex.
	for (i = 0; i < numsegs; ++i)
	{
		size_t q;

		if (segs[i].glseg)
			continue;
		if (segs[i].side != 0) // needs to be frontfacing
			continue;
		if (segs[i].v1->x != seg->v2->x)
			continue;
		if (segs[i].v1->y != seg->v2->y)
			continue;

		// Make sure you didn't already add this seg...
		for (q = 0; q < po->segCount; q++)
		{
			if (po->segs[q] == &segs[i])
				break;
		}

		if (q != po->segCount)
			continue;

		// add the new seg and recurse
		Polyobj_addSeg(po, &segs[i]);
		seg = &segs[i];

		if (!(po->flags & POF_ONESIDE))
		{
			// Find backfacings
			for (q = 0; q < numsegs; q++)
			{
				size_t r;

				if (segs[q].glseg)
					continue;
				if (segs[q].linedef != segs[i].linedef)
					continue;
				if (segs[q].side != 1)
					continue;

				for (r = 0; r < po->segCount; r++)
				{
					if (po->segs[r] == &segs[q])
						break;
				}

				if (r != po->segCount)
					continue;

				segs[q].dontrenderme = true;
				Polyobj_addSeg(po, &segs[q]);
			}
		}

		goto newseg;
	}

	// error: if we reach here, the seg search never found another seg to
	// continue the loop, and thus the polyobject is open. This isn't allowed.
	po->isBad = true;
	CONS_Debug(DBG_POLYOBJ, "Polyobject %d is not closed\n", po->id);
}

// Setup functions

static void Polyobj_spawnPolyObj(INT32 num, mobj_t *spawnSpot, INT32 id)
{
	size_t i;
	polyobj_t *po = &PolyObjects[num];

	// don't spawn a polyobject more than once
	if (po->segCount)
	{
		CONS_Debug(DBG_POLYOBJ, "Polyobj %d has more than one spawn spot", po->id);
		return;
	}

	po->id = id;

	// TODO: support customized damage somehow?
	if (spawnSpot->info->doomednum == POLYOBJ_SPAWNCRUSH_DOOMEDNUM)
		po->damage = 3;

	// set to default thrust; may be modified by attached thinkers
	// TODO: support customized thrust?
	po->thrust = FRACUNIT;
	po->spawnflags = po->flags = 0;

	// Search segs for "line start" special with tag matching this
	// polyobject's id number. If found, iterate through segs which
	// share common vertices and record them into the polyobject.
	for (i = 0; i < numsegs; ++i)
	{
		seg_t *seg = &segs[i];

		if (seg->glseg)
			continue;

		if (seg->side != 0) // needs to be frontfacing
			continue;

		if (seg->linedef->special != POLYOBJ_START_LINE)
			continue;

		if (seg->linedef->tag != po->id)
			continue;

		Polyobj_GetInfo(po); // apply extra settings if they exist!

		// save original flags and translucency to reference later for netgames!
		po->spawnflags = po->flags;
		po->spawntrans = po->translucency;

		Polyobj_findSegs(po, seg);
		break;
	}

	CONS_Debug(DBG_POLYOBJ, "PO ID: %d; Num verts: %s\n", po->id, sizeu1(po->numVertices));

	// if an error occurred above, quit processing this object
	if (po->isBad)
		return;

	// make sure array isn't empty
	if (po->segCount == 0)
	{
		po->isBad = true;
		CONS_Debug(DBG_POLYOBJ, "Polyobject %d is empty\n", po->id);
		return;
	}

	// set the polyobject's spawn spot
	po->spawnSpot.x = spawnSpot->x;
	po->spawnSpot.y = spawnSpot->y;

	// hash the polyobject by its numeric id
	if (Polyobj_GetForNum(po->id))
	{
		// bad polyobject due to id conflict
		po->isBad = true;
		CONS_Debug(DBG_POLYOBJ, "Polyobject id conflict: %d\n", id);
	}
	else
	{
		INT32 hashkey = po->id % numPolyObjects;
		po->next = PolyObjects[hashkey].first;
		PolyObjects[hashkey].first = num;
	}
}

static void Polyobj_attachToSubsec(polyobj_t *po);

// Translates the polyobject's vertices with respect to the difference between
// the anchor and spawn spots. Updates linedef bounding boxes as well.
static void Polyobj_moveToSpawnSpot(mapthing_t *anchor)
{
	polyobj_t *po;
	vertex_t  dist, sspot;
	size_t i;

	if (!(po = Polyobj_GetForNum(anchor->angle)))
	{
		CONS_Debug(DBG_POLYOBJ, "Bad polyobject %d for anchor point\n", anchor->angle);
		return;
	}

	// don't move any bad polyobject that may have gotten through
	if (po->isBad)
		return;

	// don't move any polyobject more than once
	if (po->attached)
	{
		CONS_Debug(DBG_POLYOBJ, "Polyobj %d has more than one anchor\n", po->id);
		return;
	}

	sspot.x = po->spawnSpot.x;
	sspot.y = po->spawnSpot.y;

	// calculate distance from anchor to spawn spot
	dist.x = (anchor->x << FRACBITS) - sspot.x;
	dist.y = (anchor->y << FRACBITS) - sspot.y;

	// update linedef bounding boxes
	for (i = 0; i < po->numLines; ++i)
		Polyobj_bboxSub(po->lines[i]->bbox, &dist);

	// translate vertices and record original coordinates relative to spawn spot
	for (i = 0; i < po->numVertices; ++i)
	{
		Polyobj_vecSub(po->vertices[i], &dist);

		Polyobj_vecSub2(&(po->origVerts[i]), po->vertices[i], &sspot);
	}

	// attach to subsector
	Polyobj_attachToSubsec(po);
}

// Attaches a polyobject to its appropriate subsector.
static void Polyobj_attachToSubsec(polyobj_t *po)
{
	subsector_t  *ss;
	fixed_t center_x = 0, center_y = 0;
	fixed_t numVertices;
	size_t i;

	// never attach a bad polyobject
	if (po->isBad)
		return;

	numVertices = (fixed_t)(po->numVertices*FRACUNIT);

	for (i = 0; i < po->numVertices; ++i)
	{
		center_x += FixedDiv(po->vertices[i]->x, numVertices);
		center_y += FixedDiv(po->vertices[i]->y, numVertices);
	}

	po->centerPt.x = center_x;
	po->centerPt.y = center_y;

	ss = R_PointInSubsector(po->centerPt.x, po->centerPt.y);

	M_DLListInsert(&po->link, (mdllistitem_t **)(void *)(&ss->polyList));

#ifdef R_LINKEDPORTALS
	// set spawnSpot's groupid for correct portal sound behavior
	po->spawnSpot.groupid = ss->sector->groupid;
#endif

	po->attached = true;
}

// Removes a polyobject from the subsector to which it is attached.
static void Polyobj_removeFromSubsec(polyobj_t *po)
{
	if (po->attached)
	{
		M_DLListRemove(&po->link);
		po->attached = false;
	}
}

// Blockmap Functions

// Retrieves a polymaplink object from the free list or creates a new one.
static polymaplink_t *Polyobj_getLink(void)
{
	polymaplink_t *l;

	if (bmap_freelist)
	{
		l = bmap_freelist;
		bmap_freelist = (polymaplink_t *)(l->link.next);
	}
	else
	{
		l = Z_Malloc(sizeof(*l), PU_LEVEL, NULL);
		memset(l, 0, sizeof(*l));
	}

	return l;
}

// Puts a polymaplink object into the free list.
static void Polyobj_putLink(polymaplink_t *l)
{
	memset(l, 0, sizeof(*l));
	l->link.next = (mdllistitem_t *)bmap_freelist;
	bmap_freelist = l;
}

// Inserts a polyobject into the polyobject blockmap. Unlike, mobj_t's,
// polyobjects need to be linked into every blockmap cell which their
// bounding box intersects. This ensures the accurate level of clipping
// which is present with linedefs but absent from most mobj interactions.
static void Polyobj_linkToBlockmap(polyobj_t *po)
{
	fixed_t *blockbox = po->blockbox;
	size_t i;
	fixed_t x, y;

	// never link a bad polyobject or a polyobject already linked
	if (po->isBad || po->linked)
		return;

	// 2/26/06: start line box with values of first vertex, not INT32_MIN/INT32_MAX
	blockbox[BOXLEFT]   = blockbox[BOXRIGHT] = po->vertices[0]->x;
	blockbox[BOXBOTTOM] = blockbox[BOXTOP]   = po->vertices[0]->y;

	// add all vertices to the bounding box
	for (i = 1; i < po->numVertices; ++i)
		M_AddToBox(blockbox, po->vertices[i]->x, po->vertices[i]->y);

	// adjust bounding box relative to blockmap
	blockbox[BOXRIGHT]  = (unsigned)(blockbox[BOXRIGHT]  - bmaporgx) >> MAPBLOCKSHIFT;
	blockbox[BOXLEFT]   = (unsigned)(blockbox[BOXLEFT]   - bmaporgx) >> MAPBLOCKSHIFT;
	blockbox[BOXTOP]    = (unsigned)(blockbox[BOXTOP]    - bmaporgy) >> MAPBLOCKSHIFT;
	blockbox[BOXBOTTOM] = (unsigned)(blockbox[BOXBOTTOM] - bmaporgy) >> MAPBLOCKSHIFT;

	// link polyobject to every block its bounding box intersects
	for (y = blockbox[BOXBOTTOM]; y <= blockbox[BOXTOP]; ++y)
	{
		for (x = blockbox[BOXLEFT]; x <= blockbox[BOXRIGHT]; ++x)
		{
			if (!(x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight))
			{
				polymaplink_t  *l = Polyobj_getLink();

				l->po = po;

				M_DLListInsert(&l->link,
							(mdllistitem_t **)(&polyblocklinks[y*bmapwidth + x]));
			}
		}
	}

	po->linked = true;
}

// Unlinks a polyobject from all blockmap cells it intersects and returns
// its polymaplink objects to the free list.
static void Polyobj_removeFromBlockmap(polyobj_t *po)
{
	polymaplink_t *rover;
	fixed_t *blockbox = po->blockbox;
	INT32 x, y;

	// don't bother trying to unlink one that's not linked
	if (!po->linked)
		return;

	// search all cells the polyobject touches
	for (y = blockbox[BOXBOTTOM]; y <= blockbox[BOXTOP]; ++y)
	{
		for (x = blockbox[BOXLEFT]; x <= blockbox[BOXRIGHT]; ++x)
		{
			if (!(x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight))
			{
				rover = polyblocklinks[y * bmapwidth + x];

				while (rover && rover->po != po)
					rover = (polymaplink_t *)(rover->link.next);

				// polyobject not in this cell? go on to next.
				if (!rover)
					continue;

				// remove this link from the blockmap and put it on the freelist
				M_DLListRemove(&rover->link);
				Polyobj_putLink(rover);
			}
		}
	}

	po->linked = false;
}

// Movement functions

// A version of Lee's routine from p_maputl.c that accepts an mobj pointer
// argument instead of using tmthing. Returns true if the line isn't contacted
// and false otherwise.
static inline boolean Polyobj_untouched(line_t *ld, mobj_t *mo)
{
	fixed_t x, y, ptmbbox[4];

	return
		(ptmbbox[BOXRIGHT]  = (x = mo->x) + mo->radius) <= ld->bbox[BOXLEFT]   ||
		(ptmbbox[BOXLEFT]   =           x - mo->radius) >= ld->bbox[BOXRIGHT]  ||
		(ptmbbox[BOXTOP]    = (y = mo->y) + mo->radius) <= ld->bbox[BOXBOTTOM] ||
		(ptmbbox[BOXBOTTOM] =           y - mo->radius) >= ld->bbox[BOXTOP]    ||
		P_BoxOnLineSide(ptmbbox, ld) != -1;
}

// Inflicts thrust and possibly damage on a thing which has been found to be
// blocking the motion of a polyobject. The default thrust amount is only one
// unit, but the motion of the polyobject can be used to change this.
static void Polyobj_pushThing(polyobj_t *po, line_t *line, mobj_t *mo)
{
	angle_t lineangle;
	fixed_t momx, momy;
	vertex_t closest;

	// calculate angle of line and subtract 90 degrees to get normal
	lineangle = R_PointToAngle2(0, 0, line->dx, line->dy) - ANGLE_90;
	lineangle >>= ANGLETOFINESHIFT;
	momx = FixedMul(po->thrust, FINECOSINE(lineangle));
	momy = FixedMul(po->thrust, FINESINE(lineangle));
	mo->momx += momx;
	mo->momy += momy;

	// Prevent 'sticking'
	P_UnsetThingPosition(mo);
	P_ClosestPointOnLine(mo->x, mo->y, line, &closest);
	mo->x = closest.x + FixedMul(mo->radius, FINECOSINE(lineangle));
	mo->y = closest.y + FixedMul(mo->radius, FINESINE(lineangle));
	mo->x += momx;
	mo->y += momy;
	P_SetThingPosition(mo);

	// if object doesn't fit at desired location, possibly hurt it
	if (po->damage && (mo->flags & MF_SHOOTABLE))
	{
		P_CheckPosition(mo, mo->x + momx, mo->y + momy);
		mo->floorz = tmfloorz;
		mo->ceilingz = tmceilingz;
		mo->floorrover = tmfloorrover;
		mo->ceilingrover = tmceilingrover;
	}
}

// Moves an object resting on top of a polyobject by (x, y). Template function to make alteration easier.
static void Polyobj_slideThing(mobj_t *mo, fixed_t dx, fixed_t dy)
{
	if (mo->player) { // Finally this doesn't suck eggs -fickle
		fixed_t cdx, cdy;

		cdx = FixedMul(dx, FRACUNIT-CARRYFACTOR);
		cdy = FixedMul(dy, FRACUNIT-CARRYFACTOR);

		if (mo->player->onconveyor == 1)
		{
			mo->momx += cdx;
			mo->momy += cdy;

			// Multiple slides in the same tic, somehow
			mo->player->cmomx += cdx;
			mo->player->cmomy += cdy;
		}
		else
		{
			if (mo->player->onconveyor == 3)
			{
				mo->momx += cdx - mo->player->cmomx;
				mo->momy += cdy - mo->player->cmomy;
			}

			mo->player->cmomx = cdx;
			mo->player->cmomy = cdy;
		}

		dx = FixedMul(dx, FRACUNIT - mo->friction);
		dy = FixedMul(dy, FRACUNIT - mo->friction);

		if (mo->player->pflags & PF_SPINNING && (mo->player->rmomx || mo->player->rmomy) && !(mo->player->pflags & PF_STARTDASH)) {
#define SPINMULT 5184 // Consider this a substitute for properly calculating FRACUNIT-friction. I'm tired. -Red
			dx = FixedMul(dx, SPINMULT);
			dy = FixedMul(dy, SPINMULT);
#undef SPINMULT
		}

		mo->momx += dx;
		mo->momy += dy;

		mo->player->onconveyor = 1;
	} else
		P_TryMove(mo, mo->x+dx, mo->y+dy, true);
}

// Causes objects resting on top of the polyobject to 'ride' with its movement.
static void Polyobj_carryThings(polyobj_t *po, fixed_t dx, fixed_t dy)
{
	static INT32 pomovecount = 0;
	INT32 x, y;

	pomovecount++;

	if (!(po->flags & POF_SOLID))
		return;

	for (y = po->blockbox[BOXBOTTOM]; y <= po->blockbox[BOXTOP]; ++y)
	{
		for (x = po->blockbox[BOXLEFT]; x <= po->blockbox[BOXRIGHT]; ++x)
		{
			mobj_t *mo;

			if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
				continue;

			mo = blocklinks[y * bmapwidth + x];

			for (; mo; mo = mo->bnext)
			{
				if (mo->lastlook == pomovecount)
					continue;

				mo->lastlook = pomovecount;

				// Don't scroll objects that aren't affected by gravity
				if (mo->flags & MF_NOGRAVITY)
					continue;
				// (The above check used to only move MF_SOLID objects, but that's inconsistent with conveyor behavior. -Red)

				if (mo->flags & MF_NOCLIP)
					continue;

				if ((mo->eflags & MFE_VERTICALFLIP) && mo->z + mo->height != po->lines[0]->backsector->floorheight)
					continue;

				if (!(mo->eflags & MFE_VERTICALFLIP) && mo->z != po->lines[0]->backsector->ceilingheight)
					continue;

				if (!P_MobjInsidePolyobj(po, mo))
					continue;

				Polyobj_slideThing(mo, dx, dy);
			}
		}
	}
}

// Checks for things that are in the way of a polyobject line move.
// Returns true if something was hit.
static INT32 Polyobj_clipThings(polyobj_t *po, line_t *line)
{
	INT32 hitflags = 0;
	fixed_t linebox[4];
	INT32 x, y;

	if (!(po->flags & POF_SOLID))
		return hitflags;

	// adjust linedef bounding box to blockmap, extend by MAXRADIUS
	linebox[BOXLEFT]   = (unsigned)(line->bbox[BOXLEFT]   - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
	linebox[BOXRIGHT]  = (unsigned)(line->bbox[BOXRIGHT]  - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
	linebox[BOXBOTTOM] = (unsigned)(line->bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
	linebox[BOXTOP]    = (unsigned)(line->bbox[BOXTOP]    - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;

	// check all mobj blockmap cells the line contacts
	for (y = linebox[BOXBOTTOM]; y <= linebox[BOXTOP]; ++y)
	{
		for (x = linebox[BOXLEFT]; x <= linebox[BOXRIGHT]; ++x)
		{
			if (!(x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight))
			{
				mobj_t *mo = blocklinks[y * bmapwidth + x];

				for (; mo; mo = mo->bnext)
				{

					// Don't scroll objects that aren't affected by gravity
					if (mo->flags & MF_NOGRAVITY)
						continue;
					// (The above check used to only move MF_SOLID objects, but that's inconsistent with conveyor behavior. -Red)

					if (mo->flags & MF_NOCLIP)
						continue;

					if (mo->z + mo->height <= line->backsector->floorheight)
						continue;

					if (mo->z >= line->backsector->ceilingheight)
						continue;

					if (Polyobj_untouched(line, mo))
						continue;

					if (mo->flags & MF_PUSHABLE && (po->flags & POF_PUSHABLESTOP))
						hitflags |= 2;
					else
						Polyobj_pushThing(po, line, mo);

					if (mo->player && (po->lines[0]->backsector->flags & SF_TRIGGERSPECIAL_TOUCH) && !(po->flags & POF_NOSPECIALS))
						P_ProcessSpecialSector(mo->player, mo->subsector->sector, po->lines[0]->backsector);

					hitflags |= 1;
				}
			} // end if
		} // end for (y)
	} // end for (x)

	return hitflags;
}


// Moves a polyobject on the x-y plane.
static boolean Polyobj_moveXY(polyobj_t *po, fixed_t x, fixed_t y, boolean checkmobjs)
{
	size_t i;
	vertex_t vec;
	INT32 hitflags = 0;

	vec.x = x;
	vec.y = y;

	// don't move bad polyobjects
	if (po->isBad)
		return false;

	// translate vertices
	for (i = 0; i < po->numVertices; ++i)
		Polyobj_vecAdd(po->vertices[i], &vec);

	// translate each line
	for (i = 0; i < po->numLines; ++i)
		Polyobj_bboxAdd(po->lines[i]->bbox, &vec);

	if (checkmobjs)
	{
		// check for blocking things (yes, it needs to be done separately)
		for (i = 0; i < po->numLines; ++i)
			hitflags |= Polyobj_clipThings(po, po->lines[i]);
	}

	if (hitflags & 2)
	{
		// reset vertices
		for (i = 0; i < po->numVertices; ++i)
			Polyobj_vecSub(po->vertices[i], &vec);

		// reset lines that have been moved
		for (i = 0; i < po->numLines; ++i)
			Polyobj_bboxSub(po->lines[i]->bbox, &vec);
	}
	else
	{
		// translate the spawnSpot as well
		po->spawnSpot.x += vec.x;
		po->spawnSpot.y += vec.y;

		if (checkmobjs)
			Polyobj_carryThings(po, x, y);
		Polyobj_removeFromBlockmap(po); // unlink it from the blockmap
		Polyobj_removeFromSubsec(po);   // unlink it from its subsector
		Polyobj_linkToBlockmap(po);     // relink to blockmap
		Polyobj_attachToSubsec(po);     // relink to subsector
	}

	return !(hitflags & 2);
}

// Rotates a point and then translates it relative to point c.
// The formula for this can be found here:
// http://www.inversereality.org/tutorials/graphics%20programming/2dtransformations.html
// It is, of course, just a vector-matrix multiplication.
static inline void Polyobj_rotatePoint(vertex_t *v, const vector2_t *c, angle_t ang)
{
	vertex_t tmp = *v;

	v->x = FixedMul(tmp.x, FINECOSINE(ang)) - FixedMul(tmp.y,   FINESINE(ang));
	v->y = FixedMul(tmp.x,   FINESINE(ang)) + FixedMul(tmp.y, FINECOSINE(ang));

	v->x += c->x;
	v->y += c->y;
}

// Taken from P_LoadLineDefs; simply updates the linedef's dx, dy, slopetype,
// and bounding box to be consistent with its vertices.
static void Polyobj_rotateLine(line_t *ld)
{
	vertex_t *v1, *v2;

	v1 = ld->v1;
	v2 = ld->v2;

	// set dx, dy
	ld->dx = v2->x - v1->x;
	ld->dy = v2->y - v1->y;

	// determine slopetype
	ld->slopetype = !ld->dx ? ST_VERTICAL : !ld->dy ? ST_HORIZONTAL :
			((ld->dy > 0) == (ld->dx > 0)) ? ST_POSITIVE : ST_NEGATIVE;

	// update bounding box
	if (v1->x < v2->x)
	{
		ld->bbox[BOXLEFT]  = v1->x;
		ld->bbox[BOXRIGHT] = v2->x;
	}
	else
	{
		ld->bbox[BOXLEFT]  = v2->x;
		ld->bbox[BOXRIGHT] = v1->x;
	}

	if (v1->y < v2->y)
	{
		ld->bbox[BOXBOTTOM] = v1->y;
		ld->bbox[BOXTOP]    = v2->y;
	}
	else
	{
		ld->bbox[BOXBOTTOM] = v2->y;
		ld->bbox[BOXTOP]    = v1->y;
	}
}

// Causes objects resting on top of the rotating polyobject to 'ride' with its movement.
static void Polyobj_rotateThings(polyobj_t *po, vector2_t origin, angle_t delta, UINT8 turnthings)
{
	static INT32 pomovecount = 10000;
	INT32 x, y;
	angle_t deltafine = (((po->angle + delta) >> ANGLETOFINESHIFT) - (po->angle >> ANGLETOFINESHIFT)) & FINEMASK;
	// This fineshift trickery replaces the old delta>>ANGLETOFINESHIFT; doing it this way avoids loss of precision causing objects to slide off -fickle

	pomovecount++;

	if (!(po->flags & POF_SOLID))
		return;

	for (y = po->blockbox[BOXBOTTOM]; y <= po->blockbox[BOXTOP]; ++y)
	{
		for (x = po->blockbox[BOXLEFT]; x <= po->blockbox[BOXRIGHT]; ++x)
		{
			mobj_t *mo;

			if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
				continue;

			mo = blocklinks[y * bmapwidth + x];

			for (; mo; mo = mo->bnext)
			{
				if (mo->lastlook == pomovecount)
					continue;

				mo->lastlook = pomovecount;

				// Don't scroll objects that aren't affected by gravity
				if (mo->flags & MF_NOGRAVITY)
					continue;
				// (The above check used to only move MF_SOLID objects, but that's inconsistent with conveyor behavior. -Red)

				if (mo->flags & MF_NOCLIP)
					continue;

				if ((mo->eflags & MFE_VERTICALFLIP) && mo->z + mo->height != po->lines[0]->backsector->floorheight)
					continue;

				if (!(mo->eflags & MFE_VERTICALFLIP) && mo->z != po->lines[0]->backsector->ceilingheight)
					continue;

				if (!P_MobjInsidePolyobj(po, mo))
					continue;

				{
					fixed_t oldxoff, oldyoff, newxoff, newyoff;
					fixed_t c, s;

					c = FINECOSINE(deltafine);
					s = FINESINE(deltafine);

					oldxoff = mo->x-origin.x;
					oldyoff = mo->y-origin.y;

					newxoff = FixedMul(oldxoff, c)-FixedMul(oldyoff, s) - oldxoff;
					newyoff = FixedMul(oldyoff, c)+FixedMul(oldxoff, s) - oldyoff;

					Polyobj_slideThing(mo, newxoff, newyoff);

					if (turnthings == 2 || (turnthings == 1 && !mo->player)) {
						mo->angle += delta;
						P_SetPlayerAngle(mo->player, (angle_t)(mo->player->angleturn << 16) + delta);
					}
				}
			}
		}
	}
}

// Rotates a polyobject around its start point.
static boolean Polyobj_rotate(polyobj_t *po, angle_t delta, UINT8 turnthings, boolean checkmobjs)
{
	size_t i;
	angle_t angle;
	vector2_t origin;
	INT32 hitflags = 0;

	// don't move bad polyobjects
	if (po->isBad)
		return false;

	angle = (po->angle + delta) >> ANGLETOFINESHIFT;

	// point about which to rotate is the spawn spot
	origin.x = po->spawnSpot.x;
	origin.y = po->spawnSpot.y;

	// save current positions and rotate all vertices
	for (i = 0; i < po->numVertices; ++i)
	{
		po->tmpVerts[i] = *(po->vertices[i]);

		// use original pts to rotate to new position
		*(po->vertices[i]) = po->origVerts[i];

		Polyobj_rotatePoint(po->vertices[i], &origin, angle);
	}

	// rotate lines
	for (i = 0; i < po->numLines; ++i)
		Polyobj_rotateLine(po->lines[i]);

	if (checkmobjs)
	{
		// check for blocking things
		for (i = 0; i < po->numLines; ++i)
			hitflags |= Polyobj_clipThings(po, po->lines[i]);

		Polyobj_rotateThings(po, origin, delta, turnthings);
	}

	if (hitflags & 2)
	{
		// reset vertices to previous positions
		for (i = 0; i < po->numVertices; ++i)
			*(po->vertices[i]) = po->tmpVerts[i];

		// reset lines
		for (i = 0; i < po->numLines; ++i)
			Polyobj_rotateLine(po->lines[i]);
	}
	else
	{
		// update seg angles (used only by renderer)
		for (i = 0; i < po->segCount; ++i)
			po->segs[i]->angle += delta;

		// update polyobject's angle
		po->angle += delta;

		Polyobj_removeFromBlockmap(po); // unlink it from the blockmap
		Polyobj_removeFromSubsec(po);   // remove from subsector
		Polyobj_linkToBlockmap(po);     // relink to blockmap
		Polyobj_attachToSubsec(po);     // relink to subsector
	}

	return !(hitflags & 2);
}

//
// Global Functions
//

// Retrieves a polyobject by its numeric id using hashing.
// Returns NULL if no such polyobject exists.
polyobj_t *Polyobj_GetForNum(INT32 id)
{
	INT32 curidx  = PolyObjects[id % numPolyObjects].first;

	while (curidx != numPolyObjects && PolyObjects[curidx].id != id)
		curidx = PolyObjects[curidx].next;

	return curidx == numPolyObjects ? NULL : &PolyObjects[curidx];
}


// Retrieves the parenting polyobject if one exists. Returns NULL
// otherwise.
#if 0 //unused function
static polyobj_t *Polyobj_GetParent(polyobj_t *po)
{
	return (po && po->parent != -1) ? Polyobj_GetForNum(po->parent) : NULL;
}
#endif

// Iteratively retrieves the children POs of a parent,
// sorta like P_FindSectorSpecialFromTag.
static polyobj_t *Polyobj_GetChild(polyobj_t *po, INT32 *start)
{
	for (; *start < numPolyObjects; (*start)++)
	{
		if (PolyObjects[*start].parent == po->id)
			return &PolyObjects[(*start)++];
	}

	return NULL;
}

// structure used to queue up mobj pointers in Polyobj_InitLevel
typedef struct mobjqitem_s
{
	mqueueitem_t mqitem;
	mobj_t *mo;
} mobjqitem_t;

// Called at the beginning of each map after all other line and thing
// processing is finished.
void Polyobj_InitLevel(void)
{
	thinker_t   *th;
	mqueue_t    spawnqueue;
	mqueue_t    anchorqueue;
	mobjqitem_t *qitem;
	INT32 i, numAnchors = 0;
	mobj_t *mo;

	M_QueueInit(&spawnqueue);
	M_QueueInit(&anchorqueue);

	// get rid of values from previous level
	// note: as with msecnodes, it is very important to clear out the blockmap
	// node freelist, otherwise it may contain dangling pointers to old objects
	PolyObjects    = NULL;
	numPolyObjects = 0;
	bmap_freelist  = NULL;

	// run down the thinker list, count the number of spawn points, and save
	// the mobj_t pointers on a queue for use below.
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)th;

		if (mo->info->doomednum == POLYOBJ_SPAWN_DOOMEDNUM ||
			mo->info->doomednum == POLYOBJ_SPAWNCRUSH_DOOMEDNUM)
		{
			++numPolyObjects;

			qitem = malloc(sizeof(mobjqitem_t));
			memset(qitem, 0, sizeof(mobjqitem_t));
			qitem->mo = mo;
			M_QueueInsert(&(qitem->mqitem), &spawnqueue);
		}
		else if (mo->info->doomednum == POLYOBJ_ANCHOR_DOOMEDNUM)
		{
			++numAnchors;

			qitem = malloc(sizeof(mobjqitem_t));
			memset(qitem, 0, sizeof(mobjqitem_t));
			qitem->mo = mo;
			M_QueueInsert(&(qitem->mqitem), &anchorqueue);
		}
	}

	if (numPolyObjects)
	{
		// allocate the PolyObjects array
		PolyObjects = Z_Calloc(numPolyObjects * sizeof(polyobj_t),
													PU_LEVEL, NULL);

		// setup hash fields
		for (i = 0; i < numPolyObjects; ++i)
			PolyObjects[i].first = PolyObjects[i].next = numPolyObjects;

		// setup polyobjects
		for (i = 0; i < numPolyObjects; ++i)
		{
			qitem = (mobjqitem_t *)M_QueueIterator(&spawnqueue);

			Polyobj_spawnPolyObj(i, qitem->mo, qitem->mo->spawnpoint->angle);
		}

		// move polyobjects to spawn points
		for (i = 0; i < numAnchors; ++i)
		{
			qitem = (mobjqitem_t *)M_QueueIterator(&anchorqueue);

			Polyobj_moveToSpawnSpot((qitem->mo->spawnpoint));
		}

		// setup polyobject clipping
		for (i = 0; i < numPolyObjects; ++i)
			Polyobj_linkToBlockmap(&PolyObjects[i]);
	}

#if 0
	// haleyjd 02/22/06: temporary debug
	printf("DEBUG: numPolyObjects = %d\n", numPolyObjects);
	for (i = 0; i < numPolyObjects; ++i)
	{
		INT32 j;
		polyobj_t *po = &PolyObjects[i];

		printf("polyobj %d:\n", i);
		printf("id = %d, first = %d, next = %d\n", po->id, po->first, po->next);
		printf("segCount = %d, numSegsAlloc = %d\n", po->segCount, po->numSegsAlloc);
		for (j = 0; j < po->segCount; ++j)
			printf("\tseg %d: %p\n", j, po->segs[j]);
		printf("numVertices = %d, numVerticesAlloc = %d\n", po->numVertices, po->numVerticesAlloc);
		for (j = 0; j < po->numVertices; ++j)
		{
			printf("\tvtx %d: (%d, %d) / orig: (%d, %d)\n",
				j, po->vertices[j]->x>>FRACBITS, po->vertices[j]->y>>FRACBITS,
				po->origVerts[j].x>>FRACBITS, po->origVerts[j].y>>FRACBITS);
		}
		printf("numLines = %d, numLinesAlloc = %d\n", po->numLines, po->numLinesAlloc);
		for (j = 0; j < po->numLines; ++j)
			printf("\tline %d: %p\n", j, po->lines[j]);
		printf("spawnSpot = (%d, %d)\n", po->spawnSpot.x >> FRACBITS, po->spawnSpot.y >> FRACBITS);
		printf("centerPt = (%d, %d)\n", po->centerPt.x >> FRACBITS, po->centerPt.y >> FRACBITS);
		printf("attached = %d, linked = %d, validcount = %d, isBad = %d\n",
			po->attached, po->linked, po->validcount, po->isBad);
		printf("blockbox: [%d, %d, %d, %d]\n",
			po->blockbox[BOXLEFT], po->blockbox[BOXRIGHT], po->blockbox[BOXBOTTOM],
			po->blockbox[BOXTOP]);
	}
#endif

	// done with mobj queues
	M_QueueFree(&spawnqueue);
	M_QueueFree(&anchorqueue);
}

// Called when a savegame is being loaded. Rotates and translates an
// existing polyobject to its position when the game was saved.
//
// Monster Iestyn 05/04/19: Please do not interact with mobjs! You
// can cause I_Error crashes that way, and all the important mobjs are
// going to be deleted afterwards anyway.
//
void Polyobj_MoveOnLoad(polyobj_t *po, angle_t angle, fixed_t x, fixed_t y)
{
	fixed_t dx, dy;

	// first, rotate to the saved angle
	Polyobj_rotate(po, angle, false, false);

	// determine component distances to translate
	dx = x - po->spawnSpot.x;
	dy = y - po->spawnSpot.y;

	// translate
	Polyobj_moveXY(po, dx, dy, false);
}

// Thinker Functions

// Thinker function for PolyObject rotation.
void T_PolyObjRotate(polyrotate_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjRotate: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjRotate: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif

	// check for displacement due to override and reattach when possible
	if (po->thinker == NULL)
	{
		po->thinker = &th->thinker;

		// reset polyobject's thrust
		po->thrust = abs(th->speed) >> 8;
		if (po->thrust < FRACUNIT)
			po->thrust = FRACUNIT;
		else if (po->thrust > 4*FRACUNIT)
			po->thrust = 4*FRACUNIT;
	}

	// rotate by 'speed' angle per frame
	// if distance == -1, this polyobject rotates perpetually
	if (Polyobj_rotate(po, th->speed, th->turnobjs, true) && th->distance != -1)
	{
		INT32 avel = abs(th->speed);

		// decrement distance by the amount it moved
		th->distance -= avel;

		// are we at or past the destination?
		if (th->distance <= 0)
		{
			// remove thinker
			if (po->thinker == &th->thinker)
			{
				po->thinker = NULL;
				po->thrust = FRACUNIT;
			}
			P_RemoveThinker(&th->thinker);

			// TODO: notify scripts
			// TODO: sound sequence stop event
		}
		else if (th->distance < avel)
		{
			// we have less than one multiple of 'speed' left to go,
			// so change the speed so that it doesn't pass the destination
			th->speed = th->speed >= 0 ? th->distance : -th->distance;
		}
	}
}

// Calculates the speed components from the desired resultant velocity.
FUNCINLINE static ATTRINLINE void Polyobj_componentSpeed(INT32 resVel, INT32 angle,
                                            fixed_t *xVel, fixed_t *yVel)
{
	if (angle == 0)
	{
		*xVel = resVel;
		*yVel = 0;
	}
	else if (angle == (INT32)(ANGLE_90>>ANGLETOFINESHIFT))
	{
		*xVel = 0;
		*yVel = resVel;
	}
	else
	{
		*xVel = FixedMul(resVel, FINECOSINE(angle));
		*yVel = FixedMul(resVel,   FINESINE(angle));
	}
}

void T_PolyObjMove(polymove_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjMove: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjMove: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif

	// check for displacement due to override and reattach when possible
	if (po->thinker == NULL)
	{
		po->thinker = &th->thinker;

		// reset polyobject's thrust
		po->thrust = abs(th->speed) >> 3;
		if (po->thrust < FRACUNIT)
			po->thrust = FRACUNIT;
		else if (po->thrust > 4*FRACUNIT)
			po->thrust = 4*FRACUNIT;
	}

	// move the polyobject one step along its movement angle
	if (Polyobj_moveXY(po, th->momx, th->momy, true))
	{
		INT32 avel = abs(th->speed);

		// decrement distance by the amount it moved
		th->distance -= avel;

		// are we at or past the destination?
		if (th->distance <= 0)
		{
			// remove thinker
			if (po->thinker == &th->thinker)
			{
				po->thinker = NULL;
				po->thrust = FRACUNIT;
			}
			P_RemoveThinker(&th->thinker);

			// TODO: notify scripts
			// TODO: sound sequence stop event
		}
		else if (th->distance < avel)
		{
			// we have less than one multiple of 'speed' left to go,
			// so change the speed so that it doesn't pass the destination
			th->speed = th->speed >= 0 ? th->distance : -th->distance;
			Polyobj_componentSpeed(th->speed, th->angle, &th->momx, &th->momy);
		}
	}
}

static void T_MovePolyObj(polyobj_t *po, fixed_t distx, fixed_t disty, fixed_t distz)
{
	polyobj_t *child;
	INT32 start;

	Polyobj_moveXY(po, distx, disty, true);
	// TODO: use T_MovePlane
	po->lines[0]->backsector->floorheight += distz;
	po->lines[0]->backsector->ceilingheight += distz;
	// Sal: Remember to check your sectors!
	// Monster Iestyn: we only need to bother with the back sector, now that P_CheckSector automatically checks the blockmap
	//  updating objects in the front one too just added teleporting to ground bugs
	P_CheckSector(po->lines[0]->backsector, (boolean)(po->damage));
	// Apply action to mirroring polyobjects as well
	start = 0;
	while ((child = Polyobj_GetChild(po, &start)))
	{
		if (child->isBad)
			continue;

		Polyobj_moveXY(child, distx, disty, true);
		// TODO: use T_MovePlane
		child->lines[0]->backsector->floorheight += distz;
		child->lines[0]->backsector->ceilingheight += distz;
		P_CheckSector(child->lines[0]->backsector, (boolean)(child->damage));
	}
}

void T_PolyObjWaypoint(polywaypoint_t *th)
{
	mobj_t *target = NULL;
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);
	fixed_t speed = th->speed;

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjWaypoint: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjWaypoint: thinker with invalid id %d removed.", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif

	// check for displacement due to override and reattach when possible
	if (!po->thinker)
		po->thinker = &th->thinker;

	target = waypoints[th->sequence][th->pointnum];

	if (!target)
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjWaypoint: Unable to find target waypoint!\n");
		return;
	}

	// Move along the waypoint sequence until speed for the current tic is exhausted
	while (speed > 0)
	{
		mobj_t *waypoint = NULL;
		fixed_t pox, poy, poz;
		fixed_t distx, disty, distz, dist;

		// Current position of polyobject
		pox = po->centerPt.x;
		poy = po->centerPt.y;
		poz = (po->lines[0]->backsector->floorheight + po->lines[0]->backsector->ceilingheight)/2;

		// Calculate the distance between the polyobject and the waypoint
		distx = target->x - pox;
		disty = target->y - poy;
		distz = target->z - poz;
		dist = P_AproxDistance(P_AproxDistance(distx, disty), distz);

		if (dist < 1)
			dist = 1;

		// Will the polyobject overshoot its target?
		if (speed < dist)
		{
			// No. Move towards waypoint
			fixed_t momx, momy, momz;

			momx = FixedMul(FixedDiv(target->x - pox, dist), speed);
			momy = FixedMul(FixedDiv(target->y - poy, dist), speed);
			momz = FixedMul(FixedDiv(target->z - poz, dist), speed);
			T_MovePolyObj(po, momx, momy, momz);
			return;
		}
		else
		{
			// Yes. Teleport to waypoint and look for the next one
			T_MovePolyObj(po, distx, disty, distz);

			if (!th->stophere)
			{
				CONS_Debug(DBG_POLYOBJ, "Looking for next waypoint...\n");
				waypoint = (th->direction == -1) ? P_GetPreviousWaypoint(target, false) : P_GetNextWaypoint(target, false);

				if (!waypoint && th->returnbehavior == PWR_WRAP) // If specified, wrap waypoints
				{
					if (!th->continuous)
					{
						th->returnbehavior = PWR_STOP;
						th->stophere = true;
					}

					waypoint = (th->direction == -1) ? P_GetLastWaypoint(th->sequence) : P_GetFirstWaypoint(th->sequence);
				}
				else if (!waypoint && th->returnbehavior == PWR_COMEBACK) // Come back to the start
				{
					th->direction = -th->direction;

					if (!th->continuous)
						th->returnbehavior = PWR_STOP;

					waypoint = (th->direction == -1) ? P_GetPreviousWaypoint(target, false) : P_GetNextWaypoint(target, false);
				}
			}

			if (waypoint)
			{
				CONS_Debug(DBG_POLYOBJ, "Found waypoint (sequence %d, number %d).\n", waypoint->threshold, waypoint->health);

				target = waypoint;
				th->pointnum = target->health;

				// Calculate remaining speed
				speed -= dist;
			}
			else
			{
				if (!th->stophere)
					CONS_Debug(DBG_POLYOBJ, "Next waypoint not found!\n");

				if (po->thinker == &th->thinker)
					po->thinker = NULL;

				P_RemoveThinker(&th->thinker);
				return;
			}
		}
	}
}

void T_PolyDoorSlide(polyslidedoor_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyDoorSlide: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyDoorSlide: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif

	// check for displacement due to override and reattach when possible
	if (po->thinker == NULL)
	{
		po->thinker = &th->thinker;

		// reset polyobject's thrust
		po->thrust = abs(th->speed) >> 3;
		if (po->thrust < FRACUNIT)
			po->thrust = FRACUNIT;
		else if (po->thrust > 4*FRACUNIT)
			po->thrust = 4*FRACUNIT;
	}

	// count down wait period
	if (th->delayCount)
	{
		if (--th->delayCount == 0)
		{
			; // TODO: start sound sequence event
		}
		return;
	}

	// move the polyobject one step along its movement angle
	if (Polyobj_moveXY(po, th->momx, th->momy, true))
	{
		INT32 avel = abs(th->speed);

		// decrement distance by the amount it moved
		th->distance -= avel;


		// are we at or past the destination?
		if (th->distance <= 0)
		{
			// does it need to close?
			if (!th->closing)
			{
				th->closing = true;

				// reset distance and speed
				th->distance = th->initDistance;
				th->speed    = th->initSpeed;

				// start delay
				th->delayCount = th->delay;

				// reverse angle
				th->angle = th->revAngle;

				// reset component speeds
				Polyobj_componentSpeed(th->speed, th->angle, &th->momx, &th->momy);
			}
			else
			{
				// remove thinker
				if (po->thinker == &th->thinker)
				{
					po->thinker = NULL;
					po->thrust = FRACUNIT;
				}
				P_RemoveThinker(&th->thinker);
				// TODO: notify scripts
			}
			// TODO: sound sequence stop event
		}
		else if (th->distance < avel)
		{
			// we have less than one multiple of 'speed' left to go,
			// so change the speed so that it doesn't pass the
			// destination
			th->speed = th->speed >= 0
				? th->distance : -th->distance;
			Polyobj_componentSpeed(th->speed, th->angle,
				&th->momx, &th->momy);
		}
	}
	else if (th->closing && th->distance != th->initDistance)
	{
		// move was blocked, special handling required -- make it reopen
		th->distance = th->initDistance - th->distance;
		th->speed    = th->initSpeed;
		th->angle    = th->initAngle;
		Polyobj_componentSpeed(th->speed, th->angle,
			&th->momx, &th->momy);
		th->closing  = false;
		// TODO: sound sequence start event
	}
}

void T_PolyDoorSwing(polyswingdoor_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyDoorSwing: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyDoorSwing: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif

	// check for displacement due to override and reattach when possible
	if (po->thinker == NULL)
	{
		po->thinker = &th->thinker;

		// reset polyobject's thrust
		po->thrust = abs(th->speed) >> 3;
		if (po->thrust < FRACUNIT)
			po->thrust = FRACUNIT;
		else if (po->thrust > 4*FRACUNIT)
			po->thrust = 4*FRACUNIT;
	}

	// count down wait period
	if (th->delayCount)
	{
		if (--th->delayCount == 0)
		{
			; // TODO: start sound sequence event
		}
		return;
	}

	// rotate by 'speed' angle per frame
	// if distance == -1, this polyobject rotates perpetually
	if (Polyobj_rotate(po, th->speed, false, true) && th->distance != -1)
	{
		INT32 avel = abs(th->speed);

		// decrement distance by the amount it moved
		th->distance -= avel;

		// are we at or past the destination?
		if (th->distance <= 0)
		{
			// does it need to close?
			if (!th->closing)
			{
				th->closing = true;

				// reset distance and speed
				th->distance =  th->initDistance;
				th->speed    = -th->initSpeed; // reverse speed on close

				// start delay
				th->delayCount = th->delay;
			}
			else
			{
				// remove thinker
				if (po->thinker == &th->thinker)
				{
					po->thinker = NULL;
					po->thrust = FRACUNIT;
				}
				P_RemoveThinker(&th->thinker);
				// TODO: notify scripts
			}
			// TODO: sound sequence stop event
		}
		else if (th->distance < avel)
		{
			// we have less than one multiple of 'speed' left to go,
			// so change the speed so that it doesn't pass the
			// destination
			th->speed = th->speed >= 0
				? th->distance : -th->distance;
		}
	}
	else if (th->closing && th->distance != th->initDistance)
	{
		// move was blocked, special handling required -- make it reopen

		th->distance = th->initDistance - th->distance;
		th->speed    = th->initSpeed;
		th->closing  = false;

		// TODO: sound sequence start event
	}
}

// Shift a polyobject based on a control sector's heights.
void T_PolyObjDisplace(polydisplace_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);
	fixed_t newheights, delta;
	fixed_t dx, dy;

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjDisplace: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjDisplace: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif

	// check for displacement due to override and reattach when possible
	if (po->thinker == NULL)
	{
		po->thinker = &th->thinker;

		// reset polyobject's thrust
		po->thrust = FRACUNIT;
	}

	newheights = th->controlSector->floorheight+th->controlSector->ceilingheight;
	delta = newheights-th->oldHeights;

	if (!delta)
		return;

	dx = FixedMul(th->dx, delta);
	dy = FixedMul(th->dy, delta);

	if (Polyobj_moveXY(po, dx, dy, true))
		th->oldHeights = newheights;
}

// Rotate a polyobject based on a control sector's heights.
void T_PolyObjRotDisplace(polyrotdisplace_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);
	fixed_t newheights, delta;
	fixed_t rotangle;

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjRotDisplace: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjRotDisplace: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif

	// check for displacement due to override and reattach when possible
	if (po->thinker == NULL)
	{
		po->thinker = &th->thinker;

		// reset polyobject's thrust
		po->thrust = FRACUNIT;
	}

	newheights = th->controlSector->floorheight+th->controlSector->ceilingheight;
	delta = newheights-th->oldHeights;

	if (!delta)
		return;

	rotangle = FixedMul(th->rotscale, delta);

	if (Polyobj_rotate(po, FixedAngle(rotangle), th->turnobjs, true))
		th->oldHeights = newheights;
}

static inline INT32 Polyobj_AngSpeed(INT32 speed)
{
	return (speed*ANG1)>>3; // no FixedAngle()
}

// Linedef Handlers

boolean EV_DoPolyObjRotate(polyrotdata_t *prdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polyrotate_t *th;
	INT32 start;

	if (!(po = Polyobj_GetForNum(prdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjRotate: bad polyobj %d\n", prdata->polyObjNum);
		return false;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return false;

	// check for override if this polyobj already has a thinker
	if (po->thinker && !prdata->overRide)
		return false;

	// create a new thinker
	th = Z_Malloc(sizeof(polyrotate_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjRotate;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;

	// set fields
	th->polyObjNum = prdata->polyObjNum;

	// use Hexen-style byte angles for speed and distance
	th->speed = Polyobj_AngSpeed(prdata->speed * prdata->direction);

	if (prdata->distance == 360)    // 360 means perpetual
		th->distance = -1;
	else if (prdata->distance == 0) // 0 means 360 degrees
		th->distance = 0xffffffff - 1;
	else
		th->distance = FixedAngle(prdata->distance*FRACUNIT);

	// set polyobject's thrust
	po->thrust = abs(th->speed) >> 8;
	if (po->thrust < FRACUNIT)
		po->thrust = FRACUNIT;
	else if (po->thrust > 4*FRACUNIT)
		po->thrust = 4*FRACUNIT;

	// TODO: start sound sequence event

	oldpo = po;

	th->turnobjs = prdata->turnobjs;

	// apply action to mirroring polyobjects as well
	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		prdata->polyObjNum = po->id; // change id to match child polyobject's
		EV_DoPolyObjRotate(prdata);
	}

	// action was successful
	return true;
}

boolean EV_DoPolyObjMove(polymovedata_t *pmdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polymove_t *th;
	INT32 start;

	if (!(po = Polyobj_GetForNum(pmdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjMove: bad polyobj %d\n", pmdata->polyObjNum);
		return false;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return false;

	// check for override if this polyobj already has a thinker
	if (po->thinker && !pmdata->overRide)
		return false;

	// create a new thinker
	th = Z_Malloc(sizeof(polymove_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjMove;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;

	// set fields
	th->polyObjNum = pmdata->polyObjNum;
	th->distance   = pmdata->distance;
	th->speed      = pmdata->speed;
	th->angle      = pmdata->angle >> ANGLETOFINESHIFT;

	// set component speeds
	Polyobj_componentSpeed(th->speed, th->angle, &th->momx, &th->momy);

	// set polyobject's thrust
	po->thrust = abs(th->speed) >> 3;
	if (po->thrust < FRACUNIT)
		po->thrust = FRACUNIT;
	else if (po->thrust > 4*FRACUNIT)
		po->thrust = 4*FRACUNIT;

	// TODO: start sound sequence event

	oldpo = po;

	// apply action to mirroring polyobjects as well
	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		pmdata->polyObjNum = po->id; // change id to match child polyobject's
		EV_DoPolyObjMove(pmdata);
	}

	// action was successful
	return true;
}

boolean EV_DoPolyObjWaypoint(polywaypointdata_t *pwdata)
{
	polyobj_t *po;
	polywaypoint_t *th;
	mobj_t *first = NULL;

	if (!(po = Polyobj_GetForNum(pwdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjWaypoint: bad polyobj %d\n", pwdata->polyObjNum);
		return false;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return false;

	if (po->thinker) // Don't crowd out another thinker.
		return false;

	// create a new thinker
	th = Z_Malloc(sizeof(polywaypoint_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjWaypoint;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;

	// set fields
	th->polyObjNum = pwdata->polyObjNum;
	th->speed = pwdata->speed;
	th->sequence = pwdata->sequence;
	th->direction = (pwdata->flags & PWF_REVERSE) ? -1 : 1;

	th->returnbehavior = pwdata->returnbehavior;
	if (pwdata->flags & PWF_LOOP)
		th->continuous = true;
	th->stophere = false;

	// Find the first waypoint we need to use
	first = (th->direction == -1) ? P_GetLastWaypoint(th->sequence) : P_GetFirstWaypoint(th->sequence);

	if (!first)
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjWaypoint: Missing starting waypoint!\n");
		po->thinker = NULL;
		P_RemoveThinker(&th->thinker);
		return false;
	}

	// Sanity check: If all waypoints are in the same location,
	// don't allow the movement to be continuous so we don't get stuck in an infinite loop.
	if (th->continuous && P_IsDegeneratedWaypointSequence(th->sequence))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjWaypoint: All waypoints are in the same location!\n");
		th->continuous = false;
	}

	th->pointnum = first->health;

	return true;
}

static void Polyobj_doSlideDoor(polyobj_t *po, polydoordata_t *doordata)
{
	polyslidedoor_t *th;
	polyobj_t *oldpo;
	angle_t angtemp;
	INT32 start;

	// allocate and add a new slide door thinker
	th = Z_Malloc(sizeof(polyslidedoor_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyDoorSlide;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);

	// point the polyobject to this thinker
	po->thinker = &th->thinker;

	// setup fields of the thinker
	th->polyObjNum = po->id;
	th->closing = false;
	th->delay = doordata->delay;
	th->delayCount = 0;
	th->distance = th->initDistance = doordata->distance;
	th->speed = th->initSpeed = doordata->speed;

	// haleyjd: do angle reverse calculation in full precision to avoid
	// drift due to ANGLETOFINESHIFT.
	angtemp       = doordata->angle;
	th->angle     = angtemp >> ANGLETOFINESHIFT;
	th->initAngle = th->angle;
	th->revAngle  = (angtemp + ANGLE_180) >> ANGLETOFINESHIFT;

	Polyobj_componentSpeed(th->speed, th->angle, &th->momx, &th->momy);

	// set polyobject's thrust
	po->thrust = abs(th->speed) >> 3;
	if (po->thrust < FRACUNIT)
		po->thrust = FRACUNIT;
	else if (po->thrust > 4*FRACUNIT)
		po->thrust = 4*FRACUNIT;

	// TODO: sound sequence start event

	oldpo = po;

	// start action on mirroring polyobjects as well
	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
		Polyobj_doSlideDoor(po, doordata);
}

static void Polyobj_doSwingDoor(polyobj_t *po, polydoordata_t *doordata)
{
	polyswingdoor_t *th;
	polyobj_t *oldpo;
	INT32 start;

	// allocate and add a new swing door thinker
	th = Z_Malloc(sizeof(polyswingdoor_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyDoorSwing;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);

	// point the polyobject to this thinker
	po->thinker = &th->thinker;

	// setup fields of the thinker
	th->polyObjNum   = po->id;
	th->closing      = false;
	th->delay        = doordata->delay;
	th->delayCount   = 0;
	th->distance     = th->initDistance = FixedAngle(doordata->distance*FRACUNIT);
	th->speed        = Polyobj_AngSpeed(doordata->speed);
	th->initSpeed    = th->speed;

	// set polyobject's thrust
	po->thrust = abs(th->speed) >> 3;
	if (po->thrust < FRACUNIT)
		po->thrust = FRACUNIT;
	else if (po->thrust > 4*FRACUNIT)
		po->thrust = 4*FRACUNIT;

	// TODO: sound sequence start event

	oldpo = po;

	// start action on mirroring polyobjects as well
	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
		Polyobj_doSwingDoor(po, doordata);
}

boolean EV_DoPolyDoor(polydoordata_t *doordata)
{
	polyobj_t *po;

	if (!(po = Polyobj_GetForNum(doordata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyDoor: bad polyobj %d\n", doordata->polyObjNum);
		return false;
	}

	// don't allow line actions to affect bad polyobjects;
	// polyobject doors don't allow action overrides
	if (po->isBad || po->thinker)
		return false;

	switch (doordata->doorType)
	{
	case POLY_DOOR_SLIDE:
		Polyobj_doSlideDoor(po, doordata);
		break;
	case POLY_DOOR_SWING:
		Polyobj_doSwingDoor(po, doordata);
		break;
	default:
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyDoor: unknown door type %d", doordata->doorType);
		return false;
	}

	return true;
}

boolean EV_DoPolyObjDisplace(polydisplacedata_t *prdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polydisplace_t *th;
	INT32 start;

	if (!(po = Polyobj_GetForNum(prdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjRotate: bad polyobj %d\n", prdata->polyObjNum);
		return false;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return false;

	// create a new thinker
	th = Z_Malloc(sizeof(polydisplace_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjDisplace;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;

	// set fields
	th->polyObjNum = prdata->polyObjNum;

	th->controlSector = prdata->controlSector;
	th->oldHeights = th->controlSector->floorheight+th->controlSector->ceilingheight;

	th->dx = prdata->dx;
	th->dy = prdata->dy;

	oldpo = po;

	// apply action to mirroring polyobjects as well
	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		prdata->polyObjNum = po->id; // change id to match child polyobject's
		EV_DoPolyObjDisplace(prdata);
	}

	// action was successful
	return true;
}

boolean EV_DoPolyObjRotDisplace(polyrotdisplacedata_t *prdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polyrotdisplace_t *th;
	INT32 start;

	if (!(po = Polyobj_GetForNum(prdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjRotate: bad polyobj %d\n", prdata->polyObjNum);
		return false;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return false;

	// create a new thinker
	th = Z_Malloc(sizeof(polyrotdisplace_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjRotDisplace;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;

	// set fields
	th->polyObjNum = prdata->polyObjNum;

	th->controlSector = prdata->controlSector;
	th->oldHeights = th->controlSector->floorheight+th->controlSector->ceilingheight;

	th->rotscale = prdata->rotscale;
	th->turnobjs = prdata->turnobjs;

	oldpo = po;

	// apply action to mirroring polyobjects as well
	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		prdata->polyObjNum = po->id; // change id to match child polyobject's
		EV_DoPolyObjRotDisplace(prdata);
	}

	// action was successful
	return true;
}

void T_PolyObjFlag(polymove_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);
	size_t i;

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjFlag: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjFlag: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif

	// check for displacement due to override and reattach when possible
	if (po->thinker == NULL)
		po->thinker = &th->thinker;

	// Iterate through polyobject's vertices
	for (i = 0; i < po->numVertices/2; i++)
	{
		vertex_t vec;
		fixed_t sine = FINESINE(th->distance)*th->momx;

		Polyobj_componentSpeed(sine, th->angle, &vec.x, &vec.y);

		po->vertices[i]->x = po->tmpVerts[i].x;
		po->vertices[i]->y = po->tmpVerts[i].y;

		Polyobj_vecAdd(po->vertices[i], &vec);

		th->distance += th->speed;
		th->distance &= FINEMASK;
	}

	for (i = 0; i < po->numLines; i++)
		Polyobj_rotateLine(po->lines[i]);

	Polyobj_removeFromBlockmap(po); // unlink it from the blockmap
	Polyobj_removeFromSubsec(po);   // unlink it from its subsector
	Polyobj_linkToBlockmap(po);     // relink to blockmap
	Polyobj_attachToSubsec(po);     // relink to subsector
}

boolean EV_DoPolyObjFlag(polyflagdata_t *pfdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polymove_t *th;
	size_t i;
	INT32 start;

	if (!(po = Polyobj_GetForNum(pfdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyFlag: bad polyobj %d\n", pfdata->polyObjNum);
		return false;
	}

	// don't allow line actions to affect bad polyobjects,
	// polyobject doors don't allow action overrides
	if (po->isBad || po->thinker)
		return false;

	// Must have even # of vertices
	if (po->numVertices & 1)
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyFlag: Polyobject has odd # of vertices!\n");
		return false;
	}

	// create a new thinker
	th = Z_Malloc(sizeof(polymove_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjFlag;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;

	// set fields
	th->polyObjNum = pfdata->polyObjNum;
	th->distance   = 0;
	th->speed      = pfdata->speed;
	th->angle      = pfdata->angle;
	th->momx       = pfdata->momx;

	// save current positions
	for (i = 0; i < po->numVertices; ++i)
		po->tmpVerts[i] = *(po->vertices[i]);

	oldpo = po;

	// apply action to mirroring polyobjects as well
	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		pfdata->polyObjNum = po->id;
		EV_DoPolyObjFlag(pfdata);
	}

	// action was successful
	return true;
}

void T_PolyObjFade(polyfade_t *th)
{
	boolean stillfading = false;
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjFade: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjFade: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinker(&th->thinker);
		return;
	}
#endif

	// check for displacement due to override and reattach when possible
	if (po->thinker == NULL)
		po->thinker = &th->thinker;

	stillfading = th->ticbased ? !(--(th->timer) <= 0)
		: !((th->timer -= th->duration) <= 0);

	if (th->timer <= 0)
	{
		po->translucency = max(min(th->destvalue, NUMTRANSMAPS), 0);

		// remove thinker
		if (po->thinker == &th->thinker)
			po->thinker = NULL;
		P_RemoveThinker(&th->thinker);
	}
	else
	{
		INT16 delta = abs(th->destvalue - th->sourcevalue);
		INT32 duration = th->ticbased ? th->duration
			: abs(FixedMul(FixedDiv(256, NUMTRANSMAPS), NUMTRANSMAPS - th->destvalue)
				- FixedMul(FixedDiv(256, NUMTRANSMAPS), NUMTRANSMAPS - th->sourcevalue)); // speed-based internal counter duration: delta in 256 scale
		fixed_t factor = min(FixedDiv(duration - th->timer, duration), 1*FRACUNIT);
		if (th->destvalue < th->sourcevalue)
			po->translucency = max(min(po->translucency, th->sourcevalue - (INT16)FixedMul(delta, factor)), th->destvalue);
		else if (th->destvalue > th->sourcevalue)
			po->translucency = min(max(po->translucency, th->sourcevalue + (INT16)FixedMul(delta, factor)), th->destvalue);
	}

	if (!stillfading)
	{
		// set render flags
		if (po->translucency >= NUMTRANSMAPS) // invisible
			po->flags &= ~POF_RENDERALL;
		else
			po->flags |= (po->spawnflags & POF_RENDERALL);

		// set collision
		if (th->docollision)
		{
			if (th->destvalue > th->sourcevalue) // faded out
			{
				po->flags &= ~POF_SOLID;
				po->flags |= POF_NOSPECIALS;
			}
			else
			{
				po->flags |= (po->spawnflags & POF_SOLID);
				if (!(po->spawnflags & POF_NOSPECIALS))
					po->flags &= ~POF_NOSPECIALS;
			}
		}
	}
	else
	{
		if (po->translucency >= NUMTRANSMAPS)
			// HACK: OpenGL renders fully opaque when >= NUMTRANSMAPS
			po->translucency = NUMTRANSMAPS-1;

		po->flags |= (po->spawnflags & POF_RENDERALL);

		// set collision
		if (th->docollision)
		{
			if (th->doghostfade)
			{
				po->flags &= ~POF_SOLID;
				po->flags |= POF_NOSPECIALS;
			}
			else
			{
				po->flags |= (po->spawnflags & POF_SOLID);
				if (!(po->spawnflags & POF_NOSPECIALS))
					po->flags &= ~POF_NOSPECIALS;
			}
		}
	}
}

boolean EV_DoPolyObjFade(polyfadedata_t *pfdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polyfade_t *th;
	INT32 start;

	if (!(po = Polyobj_GetForNum(pfdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjFade: bad polyobj %d\n", pfdata->polyObjNum);
		return false;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return false;

	// already equal, nothing to do
	if (po->translucency == pfdata->destvalue)
		return true;

	if (po->thinker && po->thinker->function.acp1 == (actionf_p1)T_PolyObjFade)
		P_RemoveThinker(po->thinker);

	// create a new thinker
	th = Z_Malloc(sizeof(polyfade_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjFade;
	P_AddThinker(THINK_POLYOBJ, &th->thinker);
	po->thinker = &th->thinker;

	// set fields
	th->polyObjNum = pfdata->polyObjNum;
	th->sourcevalue = po->translucency;
	th->destvalue = pfdata->destvalue;
	th->docollision = pfdata->docollision;
	th->doghostfade = pfdata->doghostfade;

	if (pfdata->ticbased)
	{
		th->ticbased = true;
		th->timer = th->duration = abs(pfdata->speed); // pfdata->speed is duration
	}
	else
	{
		th->ticbased = false;
		th->timer = abs(FixedMul(FixedDiv(256, NUMTRANSMAPS), NUMTRANSMAPS - th->destvalue)
			- FixedMul(FixedDiv(256, NUMTRANSMAPS), NUMTRANSMAPS - th->sourcevalue)); // delta converted to 256 scale, use as internal counter
		th->duration = abs(pfdata->speed); // use th->duration as speed decrement
	}

	oldpo = po;

	// apply action to mirroring polyobjects as well
	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		pfdata->polyObjNum = po->id;
		EV_DoPolyObjFade(pfdata);
	}

	// action was successful
	return true;
}

// EOF
