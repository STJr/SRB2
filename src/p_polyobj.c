// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2006      by James Haley
// Copyright (C) 2006-2014 by Sonic Team Junior.
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

#ifdef POLYOBJECTS

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

//
// P_PointInsidePolyobj
//
// Returns TRUE if the XY point is inside the polyobject
//
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

//
// P_MobjTouchingPolyobj
//
// Returns TRUE if the mobj is touching the edge of a polyobject
//
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

//
// P_MobjInsidePolyobj
//
// Returns TRUE if the mobj is inside the polyobject
//
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

//
// P_BBoxInsidePolyobj
//
// Returns TRUE if the bbox is inside the polyobject
//
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

//
// Polyobj_GetInfo
//
// Finds the 'polyobject settings' linedef that shares the same tag
// as the polyobj linedef to get the settings for it.
//
void Polyobj_GetInfo(INT16 tag, INT32 *polyID, INT32 *mirrorID, UINT16 *exparg)
{
	INT32 i = P_FindSpecialLineFromTag(POLYINFO_SPECIALNUM, tag, -1);

	if (i == -1)
		I_Error("Polyobject (tag: %d) needs line %d for information.\n", tag, POLYINFO_SPECIALNUM);

	if (polyID)
		*polyID = lines[i].frontsector->floorheight>>FRACBITS;

	if (mirrorID)
		*mirrorID = lines[i].frontsector->special;

	if (exparg)
		*exparg = (UINT16)lines[i].frontsector->lightlevel;
}

// Reallocating array maintenance

//
// Polyobj_addVertex
//
// Adds a vertex to a polyobject's reallocating vertex arrays, provided
// that such a vertex isn't already in the array. Each vertex must only
// be translated once during polyobject movement. Keeping track of them
// this way results in much more clear and efficient code than what
// Hexen used.
//
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

//
// Polyobj_addLine
//
// Adds a linedef to a polyobject's reallocating linedefs array, provided
// that such a linedef isn't already in the array. Each linedef must only
// be adjusted once during polyobject movement. Keeping track of them
// this way provides the same benefits as for vertices.
//
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

//
// Polyobj_addSeg
//
// Adds a single seg to a polyobject's reallocating seg pointer array.
// Most polyobjects will have between 4 and 16 segs, so the array size
// begins much smaller than usual. Calls Polyobj_addVertex and Polyobj_addLine
// to add those respective structures for this seg, as well.
//
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

//
// Polyobj_findSegs
//
// This method adds segs to a polyobject by following segs from vertex to
// vertex.  The process stops when the original starting point is reached
// or if a particular search ends unexpectedly (ie, the polyobject is not
// closed).
//
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
			if (segs[s].linedef == seg->linedef
				&& segs[s].side == 1)
			{
				size_t r;
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
		if (segs[i].v1->x == seg->v2->x && segs[i].v1->y == seg->v2->y)
		{
			// Make sure you didn't already add this seg...
			size_t q;
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
				for (q = 0;  q < numsegs; q++)
				{
					if (segs[q].linedef == segs[i].linedef
						&& segs[q].side == 1)
					{
						size_t r;
						for (r=0; r < po->segCount; r++)
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
			}

			goto newseg;
		}
	}

	// error: if we reach here, the seg search never found another seg to
	// continue the loop, and thus the polyobject is open. This isn't allowed.
	po->isBad = true;
	CONS_Debug(DBG_POLYOBJ, "Polyobject %d is not closed\n", po->id);
}

// structure used to store segs during explicit search process
typedef struct segitem_s
{
	seg_t *seg;
	INT32   num;
} segitem_t;

//
// Polyobj_segCompare
//
// Callback for qsort that compares two segitems.
//
static int Polyobj_segCompare(const void *s1, const void *s2)
{
	const segitem_t *si1 = s1;
	const segitem_t *si2 = s2;

	return si2->num - si1->num;
}

//
// Polyobj_findExplicit
//
// Searches for segs to put into a polyobject in an explicitly provided order.
//
static void Polyobj_findExplicit(polyobj_t *po)
{
	// temporary dynamic seg array
	segitem_t *segitems = NULL;
	size_t numSegItems = 0;
	size_t numSegItemsAlloc = 0;

	size_t i;

	// first loop: save off all segs with polyobject's id number
	for (i = 0; i < numsegs; ++i)
	{
		INT32 polyID, parentID;

		if (segs[i].linedef->special != POLYOBJ_EXPLICIT_LINE)
			continue;

		Polyobj_GetInfo(segs[i].linedef->tag, &polyID, &parentID, NULL);

		if (polyID == po->id && parentID > 0)
		{
			if (numSegItems >= numSegItemsAlloc)
			{
				numSegItemsAlloc = numSegItemsAlloc ? numSegItemsAlloc*2 : 4;
				segitems = Z_Realloc(segitems, numSegItemsAlloc*sizeof(segitem_t), PU_STATIC, NULL);
			}
			segitems[numSegItems].seg = &segs[i];
			segitems[numSegItems].num = parentID;
			++numSegItems;
		}
	}

	// make sure array isn't empty
	if (numSegItems == 0)
	{
		po->isBad = true;
		CONS_Debug(DBG_POLYOBJ, "Polyobject %d is empty\n", po->id);
		return;
	}

	// sort the array if necessary
	if (numSegItems >= 2)
		qsort(segitems, numSegItems, sizeof(segitem_t), Polyobj_segCompare);

	// second loop: put the sorted segs into the polyobject
	for (i = 0; i < numSegItems; ++i)
		Polyobj_addSeg(po, segitems[i].seg);

	// free the temporary array
	Z_Free(segitems);
}

// Setup functions

//
// Polyobj_spawnPolyObj
//
// Sets up a Polyobject.
//
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
	po->flags = 0;

	// 1. Search segs for "line start" special with tag matching this
	//    polyobject's id number. If found, iterate through segs which
	//    share common vertices and record them into the polyobject.
	for (i = 0; i < numsegs; ++i)
	{
		seg_t *seg = &segs[i];
		INT32 polyID, parentID;

		if (seg->linedef->special != POLYOBJ_START_LINE)
			continue;

		Polyobj_GetInfo(seg->linedef->tag, &polyID, &parentID, NULL);

		// is it a START line with this polyobject's id?
		if (polyID == po->id)
		{
			po->flags = POF_SOLID|POF_TESTHEIGHT|POF_RENDERSIDES;

			if (seg->linedef->flags & ML_EFFECT1)
				po->flags |= POF_ONESIDE;

			if (seg->linedef->flags & ML_EFFECT2)
				po->flags &= ~POF_SOLID;

			if (seg->linedef->flags & ML_EFFECT3)
				po->flags |= POF_PUSHABLESTOP;

			if (seg->linedef->flags & ML_EFFECT4)
				po->flags |= POF_RENDERPLANES;

			// TODO: Use a different linedef flag for this if we really need it!!
			// This clashes with texture tiling, also done by Effect 5 flag
			/*if (seg->linedef->flags & ML_EFFECT5)
				po->flags &= ~POF_CLIPPLANES;*/

			if (seg->linedef->flags & ML_NOCLIMB) // Has a linedef executor
				po->flags |= POF_LDEXEC;

			Polyobj_findSegs(po, seg);
			po->parent = parentID;
			if (po->parent == po->id) // do not allow a self-reference
				po->parent = -1;
			// TODO: sound sequence is in args[2]
			break;
		}
	}

	CONS_Debug(DBG_POLYOBJ, "PO ID: %d; Num verts: %s\n", po->id, sizeu1(po->numVertices));

	// if an error occurred above, quit processing this object
	if (po->isBad)
		return;

	// 2. If no such line existed in the first step, look for a seg with the
	//    "explicit" special with tag matching this polyobject's id number. If
	//    found, continue to search for all such lines, storing them in a
	//    temporary list of segs which is then copied into the polyobject in
	//    sorted order.
	if (po->segCount == 0)
	{
		UINT16 parent;
		Polyobj_findExplicit(po);
		// if an error occurred above, quit processing this object
		if (po->isBad)
			return;

		Polyobj_GetInfo(po->segs[0]->linedef->tag, NULL, NULL, &parent);
		po->parent = parent;
		if (po->parent == po->id) // do not allow a self-reference
			po->parent = -1;
		// TODO: sound sequence is in args[3]
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

//
// Polyobj_moveToSpawnSpot
//
// Translates the polyobject's vertices with respect to the difference between
// the anchor and spawn spots. Updates linedef bounding boxes as well.
//
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

//
// Polyobj_attachToSubsec
//
// Attaches a polyobject to its appropriate subsector.
//
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

//
// Polyobj_removeFromSubsec
//
// Removes a polyobject from the subsector to which it is attached.
//
static void Polyobj_removeFromSubsec(polyobj_t *po)
{
	if (po->attached)
	{
		M_DLListRemove(&po->link);
		po->attached = false;
	}
}

// Blockmap Functions

//
// Polyobj_getLink
//
// Retrieves a polymaplink object from the free list or creates a new one.
//
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

//
// Polyobj_putLink
//
// Puts a polymaplink object into the free list.
//
static void Polyobj_putLink(polymaplink_t *l)
{
	memset(l, 0, sizeof(*l));
	l->link.next = (mdllistitem_t *)bmap_freelist;
	bmap_freelist = l;
}

//
// Polyobj_linkToBlockmap
//
// Inserts a polyobject into the polyobject blockmap. Unlike, mobj_t's,
// polyobjects need to be linked into every blockmap cell which their
// bounding box intersects. This ensures the accurate level of clipping
// which is present with linedefs but absent from most mobj interactions.
//
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

//
// Polyobj_removeFromBlockmap
//
// Unlinks a polyobject from all blockmap cells it intersects and returns
// its polymaplink objects to the free list.
//
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

//
// Polyobj_untouched
//
// A version of Lee's routine from p_maputl.c that accepts an mobj pointer
// argument instead of using tmthing. Returns true if the line isn't contacted
// and false otherwise.
//
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

//
// Polyobj_pushThing
//
// Inflicts thrust and possibly damage on a thing which has been found to be
// blocking the motion of a polyobject. The default thrust amount is only one
// unit, but the motion of the polyobject can be used to change this.
//
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
	}
}

//
// Polyobj_carryThings
//
// Causes objects resting on top of the polyobject to 'ride' with its movement.
//
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

				// always push players even if not solid
				if (!((mo->flags & MF_SOLID) || mo->player))
					continue;

				if (mo->flags & MF_NOCLIP)
					continue;

				if ((mo->eflags & MFE_VERTICALFLIP) && mo->z + mo->height != po->lines[0]->backsector->floorheight)
					continue;

				if (!(mo->eflags & MFE_VERTICALFLIP) && mo->z != po->lines[0]->backsector->ceilingheight)
					continue;

				if (!P_MobjInsidePolyobj(po, mo))
					continue;

				P_TryMove(mo, mo->x+dx, mo->y+dy, true);
			}
		}
	}
}

//
// Polyobj_clipThings
//
// Checks for things that are in the way of a polyobject line move.
// Returns true if something was hit.
//
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
					// always push players even if not solid
					if (!((mo->flags & MF_SOLID) || mo->player))
						continue;

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

//
// Polyobj_moveXY
//
// Moves a polyobject on the x-y plane.
//
static boolean Polyobj_moveXY(polyobj_t *po, fixed_t x, fixed_t y)
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

	// check for blocking things (yes, it needs to be done separately)
	for (i = 0; i < po->numLines; ++i)
		hitflags |= Polyobj_clipThings(po, po->lines[i]);

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

		Polyobj_carryThings(po, x, y);
		Polyobj_removeFromBlockmap(po); // unlink it from the blockmap
		Polyobj_removeFromSubsec(po);   // unlink it from its subsector
		Polyobj_linkToBlockmap(po);     // relink to blockmap
		Polyobj_attachToSubsec(po);     // relink to subsector
	}

	return !(hitflags & 2);
}

//
// Polyobj_rotatePoint
//
// Rotates a point and then translates it relative to point c.
// The formula for this can be found here:
// http://www.inversereality.org/tutorials/graphics%20programming/2dtransformations.html
// It is, of course, just a vector-matrix multiplication.
//
static inline void Polyobj_rotatePoint(vertex_t *v, const vertex_t *c, angle_t ang)
{
	vertex_t tmp = *v;

	v->x = FixedMul(tmp.x, FINECOSINE(ang)) - FixedMul(tmp.y,   FINESINE(ang));
	v->y = FixedMul(tmp.x,   FINESINE(ang)) + FixedMul(tmp.y, FINECOSINE(ang));

	v->x += c->x;
	v->y += c->y;
}

//
// Polyobj_rotateLine
//
// Taken from P_LoadLineDefs; simply updates the linedef's dx, dy, slopetype,
// and bounding box to be consistent with its vertices.
//
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
			FixedDiv(ld->dy, ld->dx) > 0 ? ST_POSITIVE : ST_NEGATIVE;

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

//
// Polyobj_rotate
//
// Rotates a polyobject around its start point.
//
static boolean Polyobj_rotate(polyobj_t *po, angle_t delta)
{
	size_t i;
	angle_t angle;
	vertex_t origin;
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

	// check for blocking things
	for (i = 0; i < po->numLines; ++i)
		hitflags |= Polyobj_clipThings(po, po->lines[i]);

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

//
// Polyobj_GetForNum
//
// Retrieves a polyobject by its numeric id using hashing.
// Returns NULL if no such polyobject exists.
//
polyobj_t *Polyobj_GetForNum(INT32 id)
{
	INT32 curidx  = PolyObjects[id % numPolyObjects].first;

	while (curidx != numPolyObjects && PolyObjects[curidx].id != id)
		curidx = PolyObjects[curidx].next;

	return curidx == numPolyObjects ? NULL : &PolyObjects[curidx];
}

//
// Polyobj_GetParent
//
// Retrieves the parenting polyobject if one exists. Returns NULL
// otherwise.
//
#if 0 //unused function
static polyobj_t *Polyobj_GetParent(polyobj_t *po)
{
	return (po && po->parent != -1) ? Polyobj_GetForNum(po->parent) : NULL;
}
#endif

//
// Polyobj_GetChild
//
// Iteratively retrieves the children POs of a parent,
// sorta like P_FindSectorSpecialFromTag.
//
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

//
// Polyobj_InitLevel
//
// Called at the beginning of each map after all other line and thing
// processing is finished.
//
void Polyobj_InitLevel(void)
{
	thinker_t   *th;
	mqueue_t    spawnqueue;
	mqueue_t    anchorqueue;
	mobjqitem_t *qitem;
	INT32 i, numAnchors = 0;

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
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_MobjThinker)
		{
			mobj_t *mo = (mobj_t *)th;

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

//
// Polyobj_MoveOnLoad
//
// Called when a savegame is being loaded. Rotates and translates an
// existing polyobject to its position when the game was saved.
//
void Polyobj_MoveOnLoad(polyobj_t *po, angle_t angle, fixed_t x, fixed_t y)
{
	fixed_t dx, dy;

	// first, rotate to the saved angle
	Polyobj_rotate(po, angle);

	// determine component distances to translate
	dx = x - po->spawnSpot.x;
	dy = y - po->spawnSpot.y;

	// translate
	Polyobj_moveXY(po, dx, dy);
}

// Thinker Functions

//
// T_PolyObjRotate
//
// Thinker function for PolyObject rotation.
//
void T_PolyObjRotate(polyrotate_t *th)
{
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjRotate: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjRotate: thinker with invalid id %d removed.\n", th->polyObjNum);
		P_RemoveThinkerDelayed(&th->thinker);
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
	if (Polyobj_rotate(po, th->speed) && th->distance != -1)
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

//
// Polyobj_componentSpeed
//
// Calculates the speed components from the desired resultant velocity.
//
FUNCINLINE static ATTRINLINE void Polyobj_componentSpeed(INT32 resVel, INT32 angle,
                                            fixed_t *xVel, fixed_t *yVel)
{
	*xVel = FixedMul(resVel, FINECOSINE(angle));
	*yVel = FixedMul(resVel,   FINESINE(angle));
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
		P_RemoveThinkerDelayed(&th->thinker);
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
	if (Polyobj_moveXY(po, th->momx, th->momy))
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

//
// T_PolyObjWaypoint
//
// Kinda like 'Zoom Tubes for PolyObjects'
//
void T_PolyObjWaypoint(polywaypoint_t *th)
{
	mobj_t *mo2;
	mobj_t *target = NULL;
	mobj_t *waypoint = NULL;
	thinker_t *wp;
	fixed_t adjustx, adjusty, adjustz;
	fixed_t momx, momy, momz, dist;
	INT32 start;
	polyobj_t *po = Polyobj_GetForNum(th->polyObjNum);
	polyobj_t *oldpo = po;

	if (!po)
#ifdef RANGECHECK
		I_Error("T_PolyObjWaypoint: thinker has invalid id %d\n", th->polyObjNum);
#else
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjWaypoint: thinker with invalid id %d removed.", th->polyObjNum);
		P_RemoveThinkerDelayed(&th->thinker);
		return;
	}
#endif

	// check for displacement due to override and reattach when possible
	if (po->thinker == NULL)
		po->thinker = &th->thinker;

	// Find out target first.
	// We redo this each tic to make savegame compatibility easier.
	for (wp = thinkercap.next; wp != &thinkercap; wp = wp->next)
	{
		if (wp->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
			continue;

		mo2 = (mobj_t *)wp;

		if (mo2->type != MT_TUBEWAYPOINT)
			continue;

		if (mo2->threshold == th->sequence && mo2->health == th->pointnum)
		{
			target = mo2;
			break;
		}
	}

	if (!target)
	{
		CONS_Debug(DBG_POLYOBJ, "T_PolyObjWaypoint: Unable to find target waypoint!\n");
		return;
	}

	// Compensate for position offset
	adjustx = po->centerPt.x + th->diffx;
	adjusty = po->centerPt.y + th->diffy;
	adjustz = po->lines[0]->backsector->floorheight + (po->lines[0]->backsector->ceilingheight - po->lines[0]->backsector->floorheight)/2 + th->diffz;

	dist = P_AproxDistance(P_AproxDistance(target->x - adjustx, target->y - adjusty), target->z - adjustz);

	if (dist < 1)
		dist = 1;

	momx = FixedMul(FixedDiv(target->x - adjustx, dist), (th->speed));
	momy = FixedMul(FixedDiv(target->y - adjusty, dist), (th->speed));
	momz = FixedMul(FixedDiv(target->z - adjustz, dist), (th->speed));

	// Calculate the distance between the polyobject and the waypoint
	// 'dist' already equals this.

	// Will the polyobject be FURTHER away if the momx/momy/momz is added to
	// its current coordinates, or closer? (shift down to fracunits to avoid approximation errors)
	if (dist>>FRACBITS <= P_AproxDistance(P_AproxDistance(target->x - adjustx - momx, target->y - adjusty - momy), target->z - adjustz - momz)>>FRACBITS)
	{
		// If further away, set XYZ of polyobject to waypoint location
		fixed_t amtx, amty;
		amtx = (target->x - th->diffx) - po->centerPt.x;
		amty = (target->y - th->diffy) - po->centerPt.y;
		Polyobj_moveXY(po, amtx, amty);
		// TODO: use T_MovePlane
		amtx = (po->lines[0]->backsector->ceilingheight - po->lines[0]->backsector->floorheight)/2;
		po->lines[0]->backsector->floorheight = target->z - amtx;
		po->lines[0]->backsector->ceilingheight = target->z + amtx;

		// Apply action to mirroring polyobjects as well
		start = 0;
		while ((po = Polyobj_GetChild(oldpo, &start)))
		{
			if (po->isBad)
				continue;

			Polyobj_moveXY(po, amtx, amty);
			// TODO: use T_MovePlane
			amtx = (po->lines[0]->backsector->ceilingheight - po->lines[0]->backsector->floorheight)/2;
			po->lines[0]->backsector->floorheight = target->z - amtx;
			po->lines[0]->backsector->ceilingheight = target->z + amtx;
		}

		po = oldpo;

		if (!th->stophere)
		{
			CONS_Debug(DBG_POLYOBJ, "Looking for next waypoint...\n");

			// Find next waypoint
			for (wp = thinkercap.next; wp != &thinkercap; wp = wp->next)
			{
				if (wp->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
					continue;

				mo2 = (mobj_t *)wp;

				if (mo2->type != MT_TUBEWAYPOINT)
					continue;

				if (mo2->threshold == th->sequence)
				{
					if (th->direction == -1)
					{
						if (mo2->health == target->health - 1)
						{
							waypoint = mo2;
							break;
						}
					}
					else
					{
						if (mo2->health == target->health + 1)
						{
							waypoint = mo2;
							break;
						}
					}
				}
			}

			if (!waypoint && th->wrap) // If specified, wrap waypoints
			{
				if (!th->continuous)
				{
					th->wrap = 0;
					th->stophere = true;
				}

				for (wp = thinkercap.next; wp != &thinkercap; wp = wp->next)
				{
					if (wp->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
						continue;

					mo2 = (mobj_t *)wp;

					if (mo2->type != MT_TUBEWAYPOINT)
						continue;

					if (mo2->threshold == th->sequence)
					{
						if (th->direction == -1)
						{
							if (waypoint == NULL)
								waypoint = mo2;
							else if (mo2->health > waypoint->health)
								waypoint = mo2;
						}
						else
						{
							if (mo2->health == 0)
							{
								waypoint = mo2;
								break;
							}
						}
					}
				}
			}
			else if (!waypoint && th->comeback) // Come back to the start
			{
				th->direction = -th->direction;

				if (!th->continuous)
					th->comeback = false;

				for (wp = thinkercap.next; wp != &thinkercap; wp = wp->next)
				{
					if (wp->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
						continue;

					mo2 = (mobj_t *)wp;

					if (mo2->type != MT_TUBEWAYPOINT)
						continue;

					if (mo2->threshold == th->sequence)
					{
						if (th->direction == -1)
						{
							if (mo2->health == target->health - 1)
							{
								waypoint = mo2;
								break;
							}
						}
						else
						{
							if (mo2->health == target->health + 1)
							{
								waypoint = mo2;
								break;
							}
						}
					}
				}
			}
		}

		if (waypoint)
		{
			CONS_Debug(DBG_POLYOBJ, "Found waypoint (sequence %d, number %d).\n", waypoint->threshold, waypoint->health);

			target = waypoint;
			th->pointnum = target->health;

			// calculate MOMX/MOMY/MOMZ for next waypoint
			// change slope
			dist = P_AproxDistance(P_AproxDistance(target->x - adjustx, target->y - adjusty), target->z - adjustz);

			if (dist < 1)
				dist = 1;

			momx = FixedMul(FixedDiv(target->x - adjustx, dist), (th->speed));
			momy = FixedMul(FixedDiv(target->y - adjusty, dist), (th->speed));
			momz = FixedMul(FixedDiv(target->z - adjustz, dist), (th->speed));
		}
		else
		{
			momx = momy = momz = 0;

			if (!th->stophere)
				CONS_Debug(DBG_POLYOBJ, "Next waypoint not found!\n");

			if (po->thinker == &th->thinker)
				po->thinker = NULL;

			P_RemoveThinker(&th->thinker);
			return;
		}
	}
	else
	{
		// momx/momy/momz already equals the right speed
	}

	// Move the polyobject
	Polyobj_moveXY(po, momx, momy);
	// TODO: use T_MovePlane
	po->lines[0]->backsector->floorheight += momz;
	po->lines[0]->backsector->ceilingheight += momz;

	// Apply action to mirroring polyobjects as well
	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
	{
		if (po->isBad)
			continue;

		Polyobj_moveXY(po, momx, momy);
		// TODO: use T_MovePlane
		po->lines[0]->backsector->floorheight += momz;
		po->lines[0]->backsector->ceilingheight += momz;
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
		P_RemoveThinkerDelayed(&th->thinker);
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
	if (Polyobj_moveXY(po, th->momx, th->momy))
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
		P_RemoveThinkerDelayed(&th->thinker);
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
	if (Polyobj_rotate(po, th->speed) && th->distance != -1)
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

static inline INT32 Polyobj_AngSpeed(INT32 speed)
{
	return (speed*ANG1)>>3; // no FixedAngle()
}

// Linedef Handlers

INT32 EV_DoPolyObjRotate(polyrotdata_t *prdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polyrotate_t *th;
	INT32 start;

	if (!(po = Polyobj_GetForNum(prdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjRotate: bad polyobj %d\n", prdata->polyObjNum);
		return 0;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return 0;

	// check for override if this polyobj already has a thinker
	if (po->thinker && !prdata->overRide)
		return 0;

	// create a new thinker
	th = Z_Malloc(sizeof(polyrotate_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjRotate;
	P_AddThinker(&th->thinker);
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

	// apply action to mirroring polyobjects as well
	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
		EV_DoPolyObjRotate(prdata);

	// action was successful
	return 1;
}

INT32 EV_DoPolyObjMove(polymovedata_t *pmdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polymove_t *th;
	INT32 start;

	if (!(po = Polyobj_GetForNum(pmdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjMove: bad polyobj %d\n", pmdata->polyObjNum);
		return 0;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return 0;

	// check for override if this polyobj already has a thinker
	if (po->thinker && !pmdata->overRide)
		return 0;

	// create a new thinker
	th = Z_Malloc(sizeof(polymove_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjMove;
	P_AddThinker(&th->thinker);
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
		EV_DoPolyObjMove(pmdata);

	// action was successful
	return 1;
}

INT32 EV_DoPolyObjWaypoint(polywaypointdata_t *pwdata)
{
	polyobj_t *po;
	polywaypoint_t *th;
	mobj_t *mo2;
	mobj_t *first = NULL;
	mobj_t *last = NULL;
	mobj_t *target = NULL;
	thinker_t *wp;

	if (!(po = Polyobj_GetForNum(pwdata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjWaypoint: bad polyobj %d\n", pwdata->polyObjNum);
		return 0;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return 0;

	if (po->thinker) // Don't crowd out another thinker.
		return 0;

	// create a new thinker
	th = Z_Malloc(sizeof(polywaypoint_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjWaypoint;
	P_AddThinker(&th->thinker);
	po->thinker = &th->thinker;

	// set fields
	th->polyObjNum = pwdata->polyObjNum;
	th->speed = pwdata->speed;
	th->sequence = pwdata->sequence; // Used to specify sequence #
	if (pwdata->reverse)
		th->direction = -1;
	else
		th->direction = 1;

	th->comeback = pwdata->comeback;
	th->continuous = pwdata->continuous;
	th->wrap = pwdata->wrap;
	th->stophere = false;

	// Find the first waypoint we need to use
	for (wp = thinkercap.next; wp != &thinkercap; wp = wp->next)
	{
		if (wp->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
			continue;

		mo2 = (mobj_t *)wp;

		if (mo2->type != MT_TUBEWAYPOINT)
			continue;

		if (mo2->threshold == th->sequence)
		{
			if (th->direction == -1) // highest waypoint #
			{
				if (mo2->health == 0)
					last = mo2;
				else
				{
					if (first == NULL)
						first = mo2;
					else if (mo2->health > first->health)
						first = mo2;
				}
			}
			else // waypoint 0
			{
				if (mo2->health == 0)
					first = mo2;
				else
				{
					if (last == NULL)
						last = mo2;
					else if (mo2->health > last->health)
						last = mo2;
				}
			}
		}
	}

	if (!first)
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjWaypoint: Missing starting waypoint!\n");
		po->thinker = NULL;
		P_RemoveThinker(&th->thinker);
		return 0;
	}

	// Set diffx, diffy, diffz
	// Put these at 0 for now...might not be needed after all.
	th->diffx = 0;//first->x - po->centerPt.x;
	th->diffy = 0;//first->y - po->centerPt.y;
	th->diffz = 0;//first->z - (po->lines[0]->backsector->floorheight + (po->lines[0]->backsector->ceilingheight - po->lines[0]->backsector->floorheight)/2);

	if (last->x == po->centerPt.x
		&& last->y == po->centerPt.y
		&& last->z == (po->lines[0]->backsector->floorheight + (po->lines[0]->backsector->ceilingheight - po->lines[0]->backsector->floorheight)/2))
	{
		// Already at the destination point...
		if (!th->wrap)
		{
			po->thinker = NULL;
			P_RemoveThinker(&th->thinker);
		}
	}

	// Find the actual target movement waypoint
	target = first;
	/*for (wp = thinkercap.next; wp != &thinkercap; wp = wp->next)
	{
		if (wp->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
			continue;

		mo2 = (mobj_t *)wp;

		if (mo2->type != MT_TUBEWAYPOINT)
			continue;

		if (mo2->threshold == th->sequence)
		{
			if (th->direction == -1) // highest waypoint #
			{
				if (mo2->health == first->health - 1)
				{
					target = mo2;
					break;
				}
			}
			else // waypoint 0
			{
				if (mo2->health == first->health + 1)
				{
					target = mo2;
					break;
				}
			}
		}
	}*/

	if (!target)
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjWaypoint: Missing target waypoint!\n");
		po->thinker = NULL;
		P_RemoveThinker(&th->thinker);
		return 0;
	}

	// Set pointnum
	th->pointnum = target->health;

	// We don't deal with the mirror crap here, we'll
	// handle that in the T_Thinker function.
	return 1;
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
	P_AddThinker(&th->thinker);

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
	P_AddThinker(&th->thinker);

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

INT32 EV_DoPolyDoor(polydoordata_t *doordata)
{
	polyobj_t *po;

	if (!(po = Polyobj_GetForNum(doordata->polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyDoor: bad polyobj %d\n", doordata->polyObjNum);
		return 0;
	}

	// don't allow line actions to affect bad polyobjects;
	// polyobject doors don't allow action overrides
	if (po->isBad || po->thinker)
		return 0;

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
		return 0;
	}

	return 1;
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
		P_RemoveThinkerDelayed(&th->thinker);
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

INT32 EV_DoPolyObjFlag(line_t *pfdata)
{
	polyobj_t *po;
	polyobj_t *oldpo;
	polymove_t *th;
	size_t i;
	INT32 start;

	if (!(po = Polyobj_GetForNum(pfdata->tag)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyFlag: bad polyobj %d\n", pfdata->tag);
		return 0;
	}

	// don't allow line actions to affect bad polyobjects,
	// polyobject doors don't allow action overrides
	if (po->isBad || po->thinker)
		return 0;

	// Must have even # of vertices
	if (po->numVertices & 1)
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyFlag: Polyobject has odd # of vertices!\n");
		return 0;
	}

	// create a new thinker
	th = Z_Malloc(sizeof(polymove_t), PU_LEVSPEC, NULL);
	th->thinker.function.acp1 = (actionf_p1)T_PolyObjFlag;
	P_AddThinker(&th->thinker);
	po->thinker = &th->thinker;

	// set fields
	th->polyObjNum = pfdata->tag;
	th->distance   = 0;
	th->speed      = P_AproxDistance(pfdata->dx, pfdata->dy)>>FRACBITS;
	th->angle      = R_PointToAngle2(pfdata->v1->x, pfdata->v1->y, pfdata->v2->x, pfdata->v2->y)>>ANGLETOFINESHIFT;
	th->momx       = sides[pfdata->sidenum[0]].textureoffset>>FRACBITS;

	// save current positions
	for (i = 0; i < po->numVertices; ++i)
		po->tmpVerts[i] = *(po->vertices[i]);

	oldpo = po;

	// apply action to mirroring polyobjects as well
	start = 0;
	while ((po = Polyobj_GetChild(oldpo, &start)))
		EV_DoPolyObjFlag(pfdata);

	// action was successful
	return 1;
}

#endif // ifdef POLYOBJECTS

// EOF
