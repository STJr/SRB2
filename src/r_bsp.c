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
/// \file  r_bsp.c
/// \brief BSP traversal, handling of LineSegs for rendering

#include "doomdef.h"
#include "g_game.h"
#include "r_local.h"
#include "r_state.h"
#include "r_portal.h" // Add seg portals

#include "r_splats.h"
#include "p_local.h" // camera
#include "p_slopes.h"
#include "z_zone.h" // Check R_Prep3DFloors
#include "taglist.h"

seg_t *curline;
side_t *sidedef;
line_t *linedef;
sector_t *frontsector;
sector_t *backsector;

// very ugly realloc() of drawsegs at run-time, I upped it to 512
// instead of 256.. and someone managed to send me a level with
// 896 drawsegs! So too bad here's a limit removal a-la-Boom
drawseg_t *curdrawsegs = NULL; /**< This is used to handle multiple lists for masked drawsegs. */
drawseg_t *drawsegs = NULL;
drawseg_t *ds_p = NULL;

// indicates doors closed wrt automap bugfix:
INT32 doorclosed;

//
// R_ClearDrawSegs
//
void R_ClearDrawSegs(void)
{
	ds_p = drawsegs;
}

// Fix from boom.
#define MAXSEGS (MAXVIDWIDTH/2+1)

// newend is one past the last valid seg
static cliprange_t *newend;
static cliprange_t solidsegs[MAXSEGS];

//
// R_ClipSolidWallSegment
// Does handle solid walls,
//  e.g. single sided LineDefs (middle texture)
//  that entirely block the view.
//
static void R_ClipSolidWallSegment(INT32 first, INT32 last)
{
	cliprange_t *next;
	cliprange_t *start;

	// Find the first range that touches the range (adjacent pixels are touching).
	start = solidsegs;
	while (start->last < first - 1)
		start++;

	if (first < start->first)
	{
		if (last < start->first - 1)
		{
			// Post is entirely visible (above start), so insert a new clippost.
			R_StoreWallRange(first, last);
			next = newend;
			newend++;
			// NO MORE CRASHING!
			if (newend - solidsegs > MAXSEGS)
				I_Error("R_ClipSolidWallSegment: Solid Segs overflow!\n");

			while (next != start)
			{
				*next = *(next-1);
				next--;
			}
			next->first = first;
			next->last = last;
			return;
		}

		// There is a fragment above *start.
		R_StoreWallRange(first, start->first - 1);
		// Now adjust the clip size.
		start->first = first;
	}

	// Bottom contained in start?
	if (last <= start->last)
		return;

	next = start;
	while (last >= (next+1)->first - 1)
	{
		// There is a fragment between two posts.
		R_StoreWallRange(next->last + 1, (next+1)->first - 1);
		next++;

		if (last <= next->last)
		{
			// Bottom is contained in next.
			// Adjust the clip size.
			start->last = next->last;
			goto crunch;
		}
	}

	// There is a fragment after *next.
	R_StoreWallRange(next->last + 1, last);
	// Adjust the clip size.
	start->last = last;

	// Remove start+1 to next from the clip list, because start now covers their area.
crunch:
	if (next == start)
		return; // Post just extended past the bottom of one post.

	while (next++ != newend)
		*++start = *next; // Remove a post.

	newend = start + 1;

	// NO MORE CRASHING!
	if (newend - solidsegs > MAXSEGS)
		I_Error("R_ClipSolidWallSegment: Solid Segs overflow!\n");
}

//
// R_ClipPassWallSegment
// Clips the given range of columns, but does not include it in the clip list.
// Does handle windows, e.g. LineDefs with upper and lower texture.
//
static inline void R_ClipPassWallSegment(INT32 first, INT32 last)
{
	cliprange_t *start;

	// Find the first range that touches the range
	//  (adjacent pixels are touching).
	start = solidsegs;
	while (start->last < first - 1)
		start++;

	if (first < start->first)
	{
		if (last < start->first - 1)
		{
			// Post is entirely visible (above start).
			R_StoreWallRange(first, last);
			return;
		}

		// There is a fragment above *start.
		R_StoreWallRange(first, start->first - 1);
	}

	// Bottom contained in start?
	if (last <= start->last)
		return;

	while (last >= (start+1)->first - 1)
	{
		// There is a fragment between two posts.
		R_StoreWallRange(start->last + 1, (start+1)->first - 1);
		start++;

		if (last <= start->last)
			return;
	}

	// There is a fragment after *next.
	R_StoreWallRange(start->last + 1, last);
}

//
// R_ClearClipSegs
//
void R_ClearClipSegs(void)
{
	solidsegs[0].first = -0x7fffffff;
	solidsegs[0].last = -1;
	solidsegs[1].first = viewwidth;
	solidsegs[1].last = 0x7fffffff;
	newend = solidsegs + 2;
}
void R_PortalClearClipSegs(INT32 start, INT32 end)
{
	solidsegs[0].first = -0x7fffffff;
	solidsegs[0].last = start-1;
	solidsegs[1].first = end;
	solidsegs[1].last = 0x7fffffff;
	newend = solidsegs + 2;
}


// R_DoorClosed
//
// This function is used to fix the automap bug which
// showed lines behind closed doors simply because the door had a dropoff.
//
// It assumes that Doom has already ruled out a door being closed because
// of front-back closure (e.g. front floor is taller than back ceiling).
static INT32 R_DoorClosed(void)
{
	return

	// if door is closed because back is shut:
	backsector->ceilingheight <= backsector->floorheight

	// preserve a kind of transparent door/lift special effect:
	&& (backsector->ceilingheight >= frontsector->ceilingheight || curline->sidedef->toptexture)

	&& (backsector->floorheight <= frontsector->floorheight || curline->sidedef->bottomtexture);
}

//
// If player's view height is underneath fake floor, lower the
// drawn ceiling to be just under the floor height, and replace
// the drawn floor and ceiling textures, and light level, with
// the control sector's.
//
// Similar for ceiling, only reflected.
//
sector_t *R_FakeFlat(sector_t *sec, sector_t *tempsec, INT32 *floorlightlevel,
	INT32 *ceilinglightlevel, boolean back)
{
	if (floorlightlevel)
		*floorlightlevel = sec->floorlightsec == -1 ?
			sec->lightlevel : sectors[sec->floorlightsec].lightlevel;

	if (ceilinglightlevel)
		*ceilinglightlevel = sec->ceilinglightsec == -1 ?
			sec->lightlevel : sectors[sec->ceilinglightsec].lightlevel;

	// if (sec->midmap != -1)
	//	mapnum = sec->midmap;
	// In original colormap code, this block did not run if sec->midmap was set
	if (!sec->extra_colormap && sec->heightsec != -1)
	{
		const sector_t *s = &sectors[sec->heightsec];
		mobj_t *viewmobj = viewplayer->mo;
		INT32 heightsec;
		boolean underwater;

		if (splitscreen && viewplayer == &players[secondarydisplayplayer] && camera2.chase)
			heightsec = R_PointInSubsector(camera2.x, camera2.y)->sector->heightsec;
		else if (camera.chase && viewplayer == &players[displayplayer])
			heightsec = R_PointInSubsector(camera.x, camera.y)->sector->heightsec;
		else if (viewmobj)
			heightsec = R_PointInSubsector(viewmobj->x, viewmobj->y)->sector->heightsec;
		else
			return sec;
		underwater = heightsec != -1 && viewz <= sectors[heightsec].floorheight;

		// Replace sector being drawn, with a copy to be hacked
		*tempsec = *sec;

		// Replace floor and ceiling height with other sector's heights.
		tempsec->floorheight = s->floorheight;
		tempsec->ceilingheight = s->ceilingheight;

		if ((underwater && (tempsec->  floorheight = sec->floorheight,
			tempsec->ceilingheight = s->floorheight - 1, !back)) || viewz <= s->floorheight)
		{ // head-below-floor hack
			tempsec->floorpic = s->floorpic;
			tempsec->floor_xoffs = s->floor_xoffs;
			tempsec->floor_yoffs = s->floor_yoffs;
			tempsec->floorpic_angle = s->floorpic_angle;

			if (underwater)
			{
				if (s->ceilingpic == skyflatnum)
				{
					tempsec->floorheight = tempsec->ceilingheight+1;
					tempsec->ceilingpic = tempsec->floorpic;
					tempsec->ceiling_xoffs = tempsec->floor_xoffs;
					tempsec->ceiling_yoffs = tempsec->floor_yoffs;
					tempsec->ceilingpic_angle = tempsec->floorpic_angle;
				}
				else
				{
					tempsec->ceilingpic = s->ceilingpic;
					tempsec->ceiling_xoffs = s->ceiling_xoffs;
					tempsec->ceiling_yoffs = s->ceiling_yoffs;
					tempsec->ceilingpic_angle = s->ceilingpic_angle;
				}
			}

			tempsec->lightlevel = s->lightlevel;

			if (floorlightlevel)
				*floorlightlevel = s->floorlightsec == -1 ? s->lightlevel
					: sectors[s->floorlightsec].lightlevel;

			if (ceilinglightlevel)
				*ceilinglightlevel = s->ceilinglightsec == -1 ? s->lightlevel
					: sectors[s->ceilinglightsec].lightlevel;
		}
		else if (heightsec != -1 && viewz >= sectors[heightsec].ceilingheight
			&& sec->ceilingheight > s->ceilingheight)
		{ // Above-ceiling hack
			tempsec->ceilingheight = s->ceilingheight;
			tempsec->floorheight = s->ceilingheight + 1;

			tempsec->floorpic = tempsec->ceilingpic = s->ceilingpic;
			tempsec->floor_xoffs = tempsec->ceiling_xoffs = s->ceiling_xoffs;
			tempsec->floor_yoffs = tempsec->ceiling_yoffs = s->ceiling_yoffs;
			tempsec->floorpic_angle = tempsec->ceilingpic_angle = s->ceilingpic_angle;

			if (s->floorpic == skyflatnum) // SKYFIX?
			{
				tempsec->ceilingheight = tempsec->floorheight-1;
				tempsec->floorpic = tempsec->ceilingpic;
				tempsec->floor_xoffs = tempsec->ceiling_xoffs;
				tempsec->floor_yoffs = tempsec->ceiling_yoffs;
				tempsec->floorpic_angle = tempsec->ceilingpic_angle;
			}
			else
			{
				tempsec->ceilingheight = sec->ceilingheight;
				tempsec->floorpic = s->floorpic;
				tempsec->floor_xoffs = s->floor_xoffs;
				tempsec->floor_yoffs = s->floor_yoffs;
				tempsec->floorpic_angle = s->floorpic_angle;
			}

			tempsec->lightlevel = s->lightlevel;

			if (floorlightlevel)
				*floorlightlevel = s->floorlightsec == -1 ? s->lightlevel :
			sectors[s->floorlightsec].lightlevel;

			if (ceilinglightlevel)
				*ceilinglightlevel = s->ceilinglightsec == -1 ? s->lightlevel :
			sectors[s->ceilinglightsec].lightlevel;
		}
		sec = tempsec;
	}

	return sec;
}

boolean R_IsEmptyLine(seg_t *line, sector_t *front, sector_t *back)
{
	return (
		!line->polyseg &&
		back->ceilingpic == front->ceilingpic
		&& back->floorpic == front->floorpic
		&& back->f_slope == front->f_slope
		&& back->c_slope == front->c_slope
		&& back->lightlevel == front->lightlevel
		&& !line->sidedef->midtexture
		// Check offsets too!
		&& back->floor_xoffs == front->floor_xoffs
		&& back->floor_yoffs == front->floor_yoffs
		&& back->floorpic_angle == front->floorpic_angle
		&& back->ceiling_xoffs == front->ceiling_xoffs
		&& back->ceiling_yoffs == front->ceiling_yoffs
		&& back->ceilingpic_angle == front->ceilingpic_angle
		// Consider altered lighting.
		&& back->floorlightsec == front->floorlightsec
		&& back->ceilinglightsec == front->ceilinglightsec
		// Consider colormaps
		&& back->extra_colormap == front->extra_colormap
		&& ((!front->ffloors && !back->ffloors)
		|| Tag_Compare(&front->tags, &back->tags)));
}

//
// R_AddLine
// Clips the given segment and adds any visible pieces to the line list.
//
static void R_AddLine(seg_t *line)
{
	INT32 x1, x2;
	angle_t angle1, angle2, span, tspan;
	static sector_t tempsec;
	boolean bothceilingssky = false, bothfloorssky = false;

	portalline = false;

	if (line->polyseg && !(line->polyseg->flags & POF_RENDERSIDES))
		return;

	// big room fix
	angle1 = R_PointToAngleEx(viewx, viewy, line->v1->x, line->v1->y);
	angle2 = R_PointToAngleEx(viewx, viewy, line->v2->x, line->v2->y);
	curline = line;

	// Clip to view edges.
	span = angle1 - angle2;

	// Back side? i.e. backface culling?
	if (span >= ANGLE_180)
		return;

	// Global angle needed by segcalc.
	rw_angle1 = angle1;
	angle1 -= viewangle;
	angle2 -= viewangle;

	tspan = angle1 + clipangle;
	if (tspan > doubleclipangle)
	{
		tspan -= doubleclipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return;

		angle1 = clipangle;
	}
	tspan = clipangle - angle2;
	if (tspan > doubleclipangle)
	{
		tspan -= doubleclipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return;

		angle2 = -(signed)clipangle;
	}

	// The seg is in the view range, but not necessarily visible.
	angle1 = (angle1+ANGLE_90)>>ANGLETOFINESHIFT;
	angle2 = (angle2+ANGLE_90)>>ANGLETOFINESHIFT;
	x1 = viewangletox[angle1];
	x2 = viewangletox[angle2];

	// Does not cross a pixel?
	if (x1 >= x2)       // killough 1/31/98 -- change == to >= for robustness
		return;

	backsector = line->backsector;

	// Portal line
	if (line->linedef->special == 40 && line->side == 0)
	{
		// Render portal if recursiveness limit hasn't been reached.
		// Otherwise, render the wall normally.
		if (portalrender < cv_maxportals.value)
		{
			size_t p;
			mtag_t tag = Tag_FGet(&line->linedef->tags);
			INT32 li1 = line->linedef-lines;
			INT32 li2;

			for (p = 0; (li2 = Tag_Iterate_Lines(tag, p)) >= 0; p++)
			{
				// Skip invalid lines.
				if ((tag != Tag_FGet(&lines[li2].tags)) || (lines[li1].special != lines[li2].special) || (li1 == li2))
					continue;

				Portal_Add2Lines(li1, li2, x1, x2);
				goto clipsolid;
			}
		}
	}

	// Single sided line?
	if (!backsector)
		goto clipsolid;

	backsector = R_FakeFlat(backsector, &tempsec, NULL, NULL, true);

	doorclosed = 0;

	if (backsector->ceilingpic == skyflatnum && frontsector->ceilingpic == skyflatnum)
		bothceilingssky = true;
	if (backsector->floorpic == skyflatnum && frontsector->floorpic == skyflatnum)
		bothfloorssky = true;

	if (bothceilingssky && bothfloorssky) // everything's sky? let's save us a bit of time then
	{
		if (!line->polyseg &&
			!line->sidedef->midtexture
			&& ((!frontsector->ffloors && !backsector->ffloors)
				|| Tag_Compare(&frontsector->tags, &backsector->tags)))
			return; // line is empty, don't even bother

		goto clippass; // treat like wide open window instead
	}

	// Closed door.
	if (frontsector->f_slope || frontsector->c_slope || backsector->f_slope || backsector->c_slope)
	{
		fixed_t frontf1,frontf2, frontc1, frontc2; // front floor/ceiling ends
		fixed_t backf1, backf2, backc1, backc2; // back floor ceiling ends
#define SLOPEPARAMS(slope, end1, end2, normalheight) \
		end1 = P_GetZAt(slope, line->v1->x, line->v1->y, normalheight); \
		end2 = P_GetZAt(slope, line->v2->x, line->v2->y, normalheight);

		SLOPEPARAMS(frontsector->f_slope, frontf1, frontf2, frontsector->  floorheight)
		SLOPEPARAMS(frontsector->c_slope, frontc1, frontc2, frontsector->ceilingheight)
		SLOPEPARAMS( backsector->f_slope,  backf1,  backf2,  backsector->  floorheight)
		SLOPEPARAMS( backsector->c_slope,  backc1,  backc2,  backsector->ceilingheight)
#undef SLOPEPARAMS
		// if both ceilings are skies, consider it always "open"
		// same for floors
		if (!bothceilingssky && !bothfloorssky)
		{
			if ((backc1 <= frontf1 && backc2 <= frontf2)
				|| (backf1 >= frontc1 && backf2 >= frontc2))
			{
				goto clipsolid;
			}

			// Check for automap fix. Store in doorclosed for r_segs.c
			doorclosed = (backc1 <= backf1 && backc2 <= backf2
			&& ((backc1 >= frontc1 && backc2 >= frontc2) || curline->sidedef->toptexture)
			&& ((backf1 <= frontf1 && backf2 >= frontf2) || curline->sidedef->bottomtexture));

			if (doorclosed)
				goto clipsolid;
		}

		// Window.
		if (!bothceilingssky) // ceilings are always the "same" when sky
			if (backc1 != frontc1 || backc2 != frontc2)
				goto clippass;
		if (!bothfloorssky)	// floors are always the "same" when sky
			if (backf1 != frontf1 || backf2 != frontf2)
				goto clippass;
	}
	else
	{
		// if both ceilings are skies, consider it always "open"
		// same for floors
		if (!bothceilingssky && !bothfloorssky)
		{
			if (backsector->ceilingheight <= frontsector->floorheight
				|| backsector->floorheight >= frontsector->ceilingheight)
			{
				goto clipsolid;
			}

			// Check for automap fix. Store in doorclosed for r_segs.c
			doorclosed = R_DoorClosed();
			if (doorclosed)
				goto clipsolid;
		}

		// Window.
		if (!bothceilingssky) // ceilings are always the "same" when sky
			if (backsector->ceilingheight != frontsector->ceilingheight)
				goto clippass;
		if (!bothfloorssky)	// floors are always the "same" when sky
			if (backsector->floorheight != frontsector->floorheight)
				goto clippass;
	}

	// Reject empty lines used for triggers and special events.
	// Identical floor and ceiling on both sides, identical light levels on both sides,
	// and no middle texture.

	if (R_IsEmptyLine(line, frontsector, backsector))
		return;

clippass:
	R_ClipPassWallSegment(x1, x2 - 1);
	return;

clipsolid:
	R_ClipSolidWallSegment(x1, x2 - 1);
}

//
// R_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true if some part of the bbox might be visible.
//
//   | 0 | 1 | 2
// --+---+---+---
// 0 | 0 | 1 | 2
// 1 | 4 | 5 | 6
// 2 | 8 | 9 | A
INT32 checkcoord[12][4] =
{
	{3, 0, 2, 1},
	{3, 0, 2, 0},
	{3, 1, 2, 0},
	{0}, // UNUSED
	{2, 0, 2, 1},
	{0}, // UNUSED
	{3, 1, 3, 0},
	{0}, // UNUSED
	{2, 0, 3, 1},
	{2, 1, 3, 1},
	{2, 1, 3, 0}
};

static boolean R_CheckBBox(const fixed_t *bspcoord)
{
	angle_t angle1, angle2;
	INT32 sx1, sx2, boxpos;
	const INT32* check;
	cliprange_t *start;

	// Find the corners of the box that define the edges from current viewpoint.
	if ((boxpos = (viewx <= bspcoord[BOXLEFT] ? 0 : viewx < bspcoord[BOXRIGHT] ? 1 : 2) + (viewy >= bspcoord[BOXTOP] ? 0 : viewy > bspcoord[BOXBOTTOM] ? 4 : 8)) == 5)
		return true;

	check = checkcoord[boxpos];

	// big room fix
	angle1 = R_PointToAngleEx(viewx, viewy, bspcoord[check[0]], bspcoord[check[1]]) - viewangle;
	angle2 = R_PointToAngleEx(viewx, viewy, bspcoord[check[2]], bspcoord[check[3]]) - viewangle;

	if ((signed)angle1 < (signed)angle2)
	{
		if ((angle1 >= ANGLE_180) && (angle1 < ANGLE_270))
			angle1 = ANGLE_180-1;
		else
			angle2 = ANGLE_180;
	}

	if ((signed)angle2 >= (signed)clipangle) return false;
	if ((signed)angle1 <= -(signed)clipangle) return false;
	if ((signed)angle1 >= (signed)clipangle) angle1 = clipangle;
	if ((signed)angle2 <= -(signed)clipangle) angle2 = 0-clipangle;

	// Find the first clippost that touches the source post (adjacent pixels are touching).
	angle1 = (angle1+ANGLE_90)>>ANGLETOFINESHIFT;
	angle2 = (angle2+ANGLE_90)>>ANGLETOFINESHIFT;
	sx1 = viewangletox[angle1];
	sx2 = viewangletox[angle2];

	// Does not cross a pixel.
	if (sx1 >= sx2) return false;

	start = solidsegs;
	while (start->last < sx2)
		start++;

	if (sx1 >= start->first && sx2 <= start->last)
		return false; // The clippost contains the new span.

	return true;
}

size_t numpolys;        // number of polyobjects in current subsector
size_t num_po_ptrs;     // number of polyobject pointers allocated
polyobj_t **po_ptrs; // temp ptr array to sort polyobject pointers

//
// R_PolyobjCompare
//
// Callback for qsort that compares the z distance of two polyobjects.
// Returns the difference such that the closer polyobject will be
// sorted first.
//
static int R_PolyobjCompare(const void *p1, const void *p2)
{
	const polyobj_t *po1 = *(const polyobj_t * const *)p1;
	const polyobj_t *po2 = *(const polyobj_t * const *)p2;

	return po1->zdist - po2->zdist;
}

//
// R_SortPolyObjects
//
// haleyjd 03/03/06: Here's the REAL meat of Eternity's polyobject system.
// Hexen just figured this was impossible, but as mentioned in polyobj.c,
// it is perfectly doable within the confines of the BSP tree. Polyobjects
// must be sorted to draw in DOOM's front-to-back order within individual
// subsectors. This is a modified version of R_SortVisSprites.
//
void R_SortPolyObjects(subsector_t *sub)
{
	if (numpolys)
	{
		polyobj_t *po;
		INT32 i = 0;

		// allocate twice the number needed to minimize allocations
		if (num_po_ptrs < numpolys*2)
		{
			// use free instead realloc since faster (thanks Lee ^_^)
			free(po_ptrs);
			po_ptrs = malloc((num_po_ptrs = numpolys*2)
				* sizeof(*po_ptrs));
		}

		po = sub->polyList;

		while (po)
		{
			po->zdist = R_PointToDist2(viewx, viewy,
				po->centerPt.x, po->centerPt.y);
			po_ptrs[i++] = po;
			po = (polyobj_t *)(po->link.next);
		}

		// the polyobjects are NOT in any particular order, so use qsort
		// 03/10/06: only bother if there are actually polys to sort
		if (numpolys >= 2)
		{
			qsort(po_ptrs, numpolys, sizeof(polyobj_t *),
				R_PolyobjCompare);
		}
	}
}

//
// R_PolysegCompare
//
// Callback for qsort to sort the segs of a polyobject. Returns such that the
// closer one is sorted first. I sure hope this doesn't break anything. -Red
//
static int R_PolysegCompare(const void *p1, const void *p2)
{
	const seg_t *seg1 = *(const seg_t * const *)p1;
	const seg_t *seg2 = *(const seg_t * const *)p2;
	fixed_t dist1v1, dist1v2, dist2v1, dist2v2;

	// TODO might be a better way to get distance?
#define pdist(x, y) (FixedMul(R_PointToDist(x, y), FINECOSINE((R_PointToAngle(x, y)-viewangle)>>ANGLETOFINESHIFT))+0xFFFFFFF)
#define vxdist(v) pdist(v->x, v->y)

	dist1v1 = vxdist(seg1->v1);
	dist1v2 = vxdist(seg1->v2);
	dist2v1 = vxdist(seg2->v1);
	dist2v2 = vxdist(seg2->v2);

	if (min(dist1v1, dist1v2) != min(dist2v1, dist2v2))
		return min(dist1v1, dist1v2) - min(dist2v1, dist2v2);

	{ // That didn't work, so now let's try this.......
		fixed_t delta1, delta2, x1, y1, x2, y2;
		vertex_t *near1, *near2, *far1, *far2; // wherever you are~

		delta1 = R_PointToDist2(seg1->v1->x, seg1->v1->y, seg1->v2->x, seg1->v2->y);
		delta2 = R_PointToDist2(seg2->v1->x, seg2->v1->y, seg2->v2->x, seg2->v2->y);

		delta1 = FixedDiv(128<<FRACBITS, delta1);
		delta2 = FixedDiv(128<<FRACBITS, delta2);

		if (dist1v1 < dist1v2)
		{
			near1 = seg1->v1;
			far1 = seg1->v2;
		}
		else
		{
			near1 = seg1->v2;
			far1 = seg1->v1;
		}

		if (dist2v1 < dist2v2)
		{
			near2 = seg2->v1;
			far2 = seg2->v2;
		}
		else
		{
			near2 = seg2->v2;
			far2 = seg2->v1;
		}

		x1 = near1->x + FixedMul(far1->x-near1->x, delta1);
		y1 = near1->y + FixedMul(far1->y-near1->y, delta1);

		x2 = near2->x + FixedMul(far2->x-near2->x, delta2);
		y2 = near2->y + FixedMul(far2->y-near2->y, delta2);

		return pdist(x1, y1)-pdist(x2, y2);
	}
#undef vxdist
#undef pdist
}

//
// R_AddPolyObjects
//
// haleyjd 02/19/06
// Adds all segs in all polyobjects in the given subsector.
//
static void R_AddPolyObjects(subsector_t *sub)
{
	polyobj_t *po = sub->polyList;
	size_t i, j;

	numpolys = 0;

	// count polyobjects
	while (po)
	{
		++numpolys;
		po = (polyobj_t *)(po->link.next);
	}

	// for render stats
	ps_numpolyobjects += numpolys;

	// sort polyobjects
	R_SortPolyObjects(sub);

	// render polyobjects
	for (i = 0; i < numpolys; ++i)
	{
		qsort(po_ptrs[i]->segs, po_ptrs[i]->segCount, sizeof(seg_t *), R_PolysegCompare);
		for (j = 0; j < po_ptrs[i]->segCount; ++j)
			R_AddLine(po_ptrs[i]->segs[j]);
	}
}

//
// R_Subsector
// Determine floor/ceiling planes.
// Add sprites of things in sector.
// Draw one or more line segments.
//

drawseg_t *firstseg;

static void R_Subsector(size_t num)
{
	INT32 count, floorlightlevel, ceilinglightlevel, light;
	seg_t *line;
	subsector_t *sub;
	static sector_t tempsec; // Deep water hack
	extracolormap_t *floorcolormap;
	extracolormap_t *ceilingcolormap;
	fixed_t floorcenterz, ceilingcenterz;

#ifdef RANGECHECK
	if (num >= numsubsectors)
		I_Error("R_Subsector: ss %s with numss = %s\n", sizeu1(num), sizeu2(numsubsectors));
#endif

	// subsectors added at run-time
	if (num >= numsubsectors)
		return;

	sub = &subsectors[num];
	frontsector = sub->sector;
	count = sub->numlines;
	line = &segs[sub->firstline];

	// Deep water/fake ceiling effect.
	frontsector = R_FakeFlat(frontsector, &tempsec, &floorlightlevel, &ceilinglightlevel, false);

	floorcolormap = ceilingcolormap = frontsector->extra_colormap;

	floorcenterz   = P_GetSectorFloorZAt  (frontsector, frontsector->soundorg.x, frontsector->soundorg.y);
	ceilingcenterz = P_GetSectorCeilingZAt(frontsector, frontsector->soundorg.x, frontsector->soundorg.y);

	// Check and prep all 3D floors. Set the sector floor/ceiling light levels and colormaps.
	if (frontsector->ffloors)
	{
		if (frontsector->moved)
		{
			frontsector->numlights = sub->sector->numlights = 0;
			R_Prep3DFloors(frontsector);
			sub->sector->lightlist = frontsector->lightlist;
			sub->sector->numlights = frontsector->numlights;
			sub->sector->moved = frontsector->moved = false;
		}

		light = R_GetPlaneLight(frontsector, floorcenterz, false);
		if (frontsector->floorlightsec == -1)
			floorlightlevel = *frontsector->lightlist[light].lightlevel;
		floorcolormap = *frontsector->lightlist[light].extra_colormap;
		light = R_GetPlaneLight(frontsector, ceilingcenterz, false);
		if (frontsector->ceilinglightsec == -1)
			ceilinglightlevel = *frontsector->lightlist[light].lightlevel;
		ceilingcolormap = *frontsector->lightlist[light].extra_colormap;
	}

	sub->sector->extra_colormap = frontsector->extra_colormap;

	if (P_GetSectorFloorZAt(frontsector, viewx, viewy) < viewz
		|| frontsector->floorpic == skyflatnum
		|| (frontsector->heightsec != -1 && sectors[frontsector->heightsec].ceilingpic == skyflatnum))
	{
		floorplane = R_FindPlane(frontsector->floorheight, frontsector->floorpic, floorlightlevel,
			frontsector->floor_xoffs, frontsector->floor_yoffs, frontsector->floorpic_angle, floorcolormap, NULL, NULL, frontsector->f_slope);
	}
	else
		floorplane = NULL;

	if (P_GetSectorCeilingZAt(frontsector, viewx, viewy) > viewz
		|| frontsector->ceilingpic == skyflatnum
		|| (frontsector->heightsec != -1 && sectors[frontsector->heightsec].floorpic == skyflatnum))
	{
		ceilingplane = R_FindPlane(frontsector->ceilingheight, frontsector->ceilingpic,
			ceilinglightlevel, frontsector->ceiling_xoffs, frontsector->ceiling_yoffs, frontsector->ceilingpic_angle,
			ceilingcolormap, NULL, NULL, frontsector->c_slope);
	}
	else
		ceilingplane = NULL;

	numffloors = 0;
	ffloor[numffloors].slope = NULL;
	ffloor[numffloors].plane = NULL;
	ffloor[numffloors].polyobj = NULL;
	if (frontsector->ffloors)
	{
		ffloor_t *rover;
		fixed_t heightcheck, planecenterz;

		for (rover = frontsector->ffloors; rover && numffloors < MAXFFLOORS; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERPLANES))
				continue;

			if (frontsector->cullheight)
			{
				if (R_DoCulling(frontsector->cullheight, viewsector->cullheight, viewz, *rover->bottomheight, *rover->topheight))
				{
					rover->norender = leveltime;
					continue;
				}
			}

			ffloor[numffloors].plane = NULL;
			ffloor[numffloors].polyobj = NULL;

			heightcheck = P_GetFFloorBottomZAt(rover, viewx, viewy);

			planecenterz = P_GetFFloorBottomZAt(rover, frontsector->soundorg.x, frontsector->soundorg.y);
			if (planecenterz <= ceilingcenterz
				&& planecenterz >= floorcenterz
				&& ((viewz < heightcheck && (rover->flags & FF_BOTHPLANES || !(rover->flags & FF_INVERTPLANES)))
				|| (viewz > heightcheck && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
			{
				light = R_GetPlaneLight(frontsector, planecenterz,
					viewz < heightcheck);

				ffloor[numffloors].plane = R_FindPlane(*rover->bottomheight, *rover->bottompic,
					*frontsector->lightlist[light].lightlevel, *rover->bottomxoffs,
					*rover->bottomyoffs, *rover->bottomangle, *frontsector->lightlist[light].extra_colormap, rover, NULL, *rover->b_slope);

				ffloor[numffloors].slope = *rover->b_slope;

				// Tell the renderer this sector has slopes in it.
				if (ffloor[numffloors].slope)
					frontsector->hasslope = true;

				ffloor[numffloors].height = heightcheck;
				ffloor[numffloors].ffloor = rover;
				numffloors++;
			}
			if (numffloors >= MAXFFLOORS)
				break;
			ffloor[numffloors].plane = NULL;
			ffloor[numffloors].polyobj = NULL;

			heightcheck = P_GetFFloorTopZAt(rover, viewx, viewy);

			planecenterz = P_GetFFloorTopZAt(rover, frontsector->soundorg.x, frontsector->soundorg.y);
			if (planecenterz >= floorcenterz
				&& planecenterz <= ceilingcenterz
				&& ((viewz > heightcheck && (rover->flags & FF_BOTHPLANES || !(rover->flags & FF_INVERTPLANES)))
				|| (viewz < heightcheck && (rover->flags & FF_BOTHPLANES || rover->flags & FF_INVERTPLANES))))
			{
				light = R_GetPlaneLight(frontsector, planecenterz, viewz < heightcheck);

				ffloor[numffloors].plane = R_FindPlane(*rover->topheight, *rover->toppic,
					*frontsector->lightlist[light].lightlevel, *rover->topxoffs, *rover->topyoffs, *rover->topangle,
					*frontsector->lightlist[light].extra_colormap, rover, NULL, *rover->t_slope);

				ffloor[numffloors].slope = *rover->t_slope;

				// Tell the renderer this sector has slopes in it.
				if (ffloor[numffloors].slope)
					frontsector->hasslope = true;

				ffloor[numffloors].height = heightcheck;
				ffloor[numffloors].ffloor = rover;
				numffloors++;
			}
		}
	}

	// Polyobjects have planes, too!
	if (sub->polyList)
	{
		polyobj_t *po = sub->polyList;
		sector_t *polysec;

		while (po)
		{
			if (numffloors >= MAXFFLOORS)
				break;

			if (!(po->flags & POF_RENDERPLANES)) // Don't draw planes
			{
				po = (polyobj_t *)(po->link.next);
				continue;
			}

			polysec = po->lines[0]->backsector;
			ffloor[numffloors].plane = NULL;

			if (polysec->floorheight <= ceilingcenterz
				&& polysec->floorheight >= floorcenterz
				&& (viewz < polysec->floorheight))
			{
				light = R_GetPlaneLight(frontsector, polysec->floorheight, viewz < polysec->floorheight);
				ffloor[numffloors].plane = R_FindPlane(polysec->floorheight, polysec->floorpic,
					(light == -1 ? frontsector->lightlevel : *frontsector->lightlist[light].lightlevel), polysec->floor_xoffs, polysec->floor_yoffs,
					polysec->floorpic_angle-po->angle,
					(light == -1 ? frontsector->extra_colormap : *frontsector->lightlist[light].extra_colormap), NULL, po,
					NULL); // will ffloors be slopable eventually?

				ffloor[numffloors].height = polysec->floorheight;
				ffloor[numffloors].polyobj = po;
				ffloor[numffloors].slope = NULL;
				//ffloor[numffloors].ffloor = rover;
				po->visplane = ffloor[numffloors].plane;
				numffloors++;
			}

			if (numffloors >= MAXFFLOORS)
				break;

			ffloor[numffloors].plane = NULL;

			if (polysec->ceilingheight >= floorcenterz
				&& polysec->ceilingheight <= ceilingcenterz
				&& (viewz > polysec->ceilingheight))
			{
				light = R_GetPlaneLight(frontsector, polysec->floorheight, viewz < polysec->floorheight);
				ffloor[numffloors].plane = R_FindPlane(polysec->ceilingheight, polysec->ceilingpic,
					(light == -1 ? frontsector->lightlevel : *frontsector->lightlist[light].lightlevel), polysec->ceiling_xoffs, polysec->ceiling_yoffs, polysec->ceilingpic_angle-po->angle,
					(light == -1 ? frontsector->extra_colormap : *frontsector->lightlist[light].extra_colormap), NULL, po,
					NULL); // will ffloors be slopable eventually?

				ffloor[numffloors].polyobj = po;
				ffloor[numffloors].height = polysec->ceilingheight;
				ffloor[numffloors].slope = NULL;
				//ffloor[numffloors].ffloor = rover;
				po->visplane = ffloor[numffloors].plane;
				numffloors++;
			}

			po = (polyobj_t *)(po->link.next);
		}
	}

   // killough 9/18/98: Fix underwater slowdown, by passing real sector
   // instead of fake one. Improve sprite lighting by basing sprite
   // lightlevels on floor & ceiling lightlevels in the surrounding area.
   //
   // 10/98 killough:
   //
   // NOTE: TeamTNT fixed this bug incorrectly, messing up sprite lighting!!!
   // That is part of the 242 effect!!!  If you simply pass sub->sector to
   // the old code you will not get correct lighting for underwater sprites!!!
   // Either you must pass the fake sector and handle validcount here, on the
   // real sector, or you must account for the lighting in some other way,
   // like passing it as an argument.
	R_AddSprites(sub->sector, (floorlightlevel+ceilinglightlevel)/2);

	firstseg = NULL;

	// haleyjd 02/19/06: draw polyobjects before static lines
	if (sub->polyList)
		R_AddPolyObjects(sub);

	while (count--)
	{
//		CONS_Debug(DBG_GAMELOGIC, "Adding normal line %d...(%d)\n", line->linedef-lines, leveltime);
		if (!line->glseg && !line->polyseg) // ignore segs that belong to polyobjects
			R_AddLine(line);
		line++;
		curline = NULL; /* cph 2001/11/18 - must clear curline now we're done with it, so stuff doesn't try using it for other things */
	}
}

//
// R_Prep3DFloors
//
// This function creates the lightlists that the given sector uses to light
// floors/ceilings/walls according to the 3D floors.
void R_Prep3DFloors(sector_t *sector)
{
	ffloor_t *rover;
	ffloor_t *best;
	fixed_t bestheight, maxheight;
	INT32 count, i;
	sector_t *sec;
	pslope_t *bestslope = NULL;
	fixed_t heighttest; // I think it's better to check the Z height at the sector's center
	                    // than assume unsloped heights are accurate indicators of order in sloped sectors. -Red

	count = 1;
	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if ((rover->flags & FF_EXISTS) && (!(rover->flags & FF_NOSHADE)
			|| (rover->flags & FF_CUTLEVEL) || (rover->flags & FF_CUTSPRITES)))
		{
			count++;
			if (rover->flags & FF_DOUBLESHADOW)
				count++;
		}
	}

	if (count != sector->numlights)
	{
		Z_Free(sector->lightlist);
		sector->lightlist = Z_Calloc(sizeof (*sector->lightlist) * count, PU_LEVEL, NULL);
		sector->numlights = count;
	}
	else
		memset(sector->lightlist, 0, sizeof (lightlist_t) * count);

	heighttest = P_GetSectorCeilingZAt(sector, sector->soundorg.x, sector->soundorg.y);

	sector->lightlist[0].height = heighttest + 1;
	sector->lightlist[0].slope = sector->c_slope;
	sector->lightlist[0].lightlevel = &sector->lightlevel;
	sector->lightlist[0].caster = NULL;
	sector->lightlist[0].extra_colormap = &sector->extra_colormap;
	sector->lightlist[0].flags = 0;

	maxheight = INT32_MAX;
	for (i = 1; i < count; i++)
	{
		bestheight = INT32_MAX * -1;
		best = NULL;
		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			rover->lastlight = 0;
			if (!(rover->flags & FF_EXISTS) || (rover->flags & FF_NOSHADE
				&& !(rover->flags & FF_CUTLEVEL) && !(rover->flags & FF_CUTSPRITES)))
			continue;

			heighttest = P_GetFFloorTopZAt(rover, sector->soundorg.x, sector->soundorg.y);

			if (heighttest > bestheight && heighttest < maxheight)
			{
				best = rover;
				bestheight = heighttest;
				bestslope = *rover->t_slope;
				continue;
			}
			if (rover->flags & FF_DOUBLESHADOW) {
				heighttest = P_GetFFloorBottomZAt(rover, sector->soundorg.x, sector->soundorg.y);

				if (heighttest > bestheight
					&& heighttest < maxheight)
				{
					best = rover;
					bestheight = heighttest;
					bestslope = *rover->b_slope;
					continue;
				}
			}
		}
		if (!best)
		{
			sector->numlights = i;
			return;
		}

		sector->lightlist[i].height = maxheight = bestheight;
		sector->lightlist[i].caster = best;
		sector->lightlist[i].flags = best->flags;
		sector->lightlist[i].slope = bestslope;
		sec = &sectors[best->secnum];

		if (best->flags & FF_NOSHADE)
		{
			sector->lightlist[i].lightlevel = sector->lightlist[i-1].lightlevel;
			sector->lightlist[i].extra_colormap = sector->lightlist[i-1].extra_colormap;
		}
		else if (best->flags & FF_COLORMAPONLY)
		{
			sector->lightlist[i].lightlevel = sector->lightlist[i-1].lightlevel;
			sector->lightlist[i].extra_colormap = &sec->extra_colormap;
		}
		else
		{
			sector->lightlist[i].lightlevel = best->toplightlevel;
			sector->lightlist[i].extra_colormap = &sec->extra_colormap;
		}

		if (best->flags & FF_DOUBLESHADOW)
		{
			heighttest = P_GetFFloorBottomZAt(best, sector->soundorg.x, sector->soundorg.y);
			if (bestheight == heighttest) ///TODO: do this in a more efficient way -Red
			{
				sector->lightlist[i].lightlevel = sector->lightlist[best->lastlight].lightlevel;
				sector->lightlist[i].extra_colormap =
					sector->lightlist[best->lastlight].extra_colormap;
			}
			else
				best->lastlight = i - 1;
		}
	}
}

INT32 R_GetPlaneLight(sector_t *sector, fixed_t planeheight, boolean underside)
{
	INT32 i;

	if (!underside)
	{
		for (i = 1; i < sector->numlights; i++)
			if (sector->lightlist[i].height <= planeheight)
				return i - 1;

		return sector->numlights - 1;
	}

	for (i = 1; i < sector->numlights; i++)
		if (sector->lightlist[i].height < planeheight)
			return i - 1;

	return sector->numlights - 1;
}

//
// RenderBSPNode
// Renders all subsectors below a given node,
//  traversing subtree recursively.
// Just call with BSP root.
//
// killough 5/2/98: reformatted, removed tail recursion

void R_RenderBSPNode(INT32 bspnum)
{
	node_t *bsp;
	INT32 side;

	ps_numbspcalls++;

	while (!(bspnum & NF_SUBSECTOR))  // Found a subsector?
	{
		bsp = &nodes[bspnum];

		// Decide which side the view point is on.
		side = R_PointOnSide(viewx, viewy, bsp);
		// Recursively divide front space.
		R_RenderBSPNode(bsp->children[side]);

		// Possibly divide back space.

		if (!R_CheckBBox(bsp->bbox[side^1]))
			return;

		bspnum = bsp->children[side^1];
	}

	// PORTAL CULLING
	if (portalcullsector) {
		sector_t *sect = subsectors[bspnum & ~NF_SUBSECTOR].sector;
		if (sect != portalcullsector)
			return;
		portalcullsector = NULL;
	}

	R_Subsector(bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR);
}
