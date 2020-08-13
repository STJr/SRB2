// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2004      by Stephen McGranahan
// Copyright (C) 2015-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_slopes.c
/// \brief ZDoom + Eternity Engine Slopes, ported and enhanced by Kalaron

#include "doomdef.h"
#include "r_defs.h"
#include "r_state.h"
#include "m_bbox.h"
#include "z_zone.h"
#include "p_local.h"
#include "p_spec.h"
#include "p_slopes.h"
#include "p_setup.h"
#include "r_main.h"
#include "p_maputl.h"
#include "w_wad.h"

pslope_t *slopelist = NULL;
UINT16 slopecount = 0;

// Calculate line normal
void P_CalculateSlopeNormal(pslope_t *slope) {
	slope->normal.z = FINECOSINE(slope->zangle>>ANGLETOFINESHIFT);
	slope->normal.x = FixedMul(FINESINE(slope->zangle>>ANGLETOFINESHIFT), slope->d.x);
	slope->normal.y = FixedMul(FINESINE(slope->zangle>>ANGLETOFINESHIFT), slope->d.y);
}

/// Setup slope via 3 vertexes.
static void ReconfigureViaVertexes (pslope_t *slope, const vector3_t v1, const vector3_t v2, const vector3_t v3)
{
	vector3_t vec1, vec2;

	// Set origin.
	FV3_Copy(&slope->o, &v1);

	// Get slope's normal.
	FV3_SubEx(&v2, &v1, &vec1);
	FV3_SubEx(&v3, &v1, &vec2);

	// Set some defaults for a non-sloped "slope"
	if (vec1.z == 0 && vec2.z == 0)
	{
		slope->zangle = slope->xydirection = 0;
		slope->zdelta = slope->d.x = slope->d.y = 0;
		slope->normal.x = slope->normal.y = 0;
		slope->normal.z = FRACUNIT;
	}
	else
	{
		/// \note Using fixed point for vectorial products easily leads to overflows so we work around by downscaling them.
		fixed_t m = max(
			max(max(abs(vec1.x), abs(vec1.y)), abs(vec1.z)),
			max(max(abs(vec2.x), abs(vec2.y)), abs(vec2.z))
		) >> 5; // shifting right by 5 is good enough.

		FV3_Cross(
				FV3_Divide(&vec1, m),
				FV3_Divide(&vec2, m),
				&slope->normal
				);

		// NOTE: FV3_Magnitude() doesn't work properly in some cases, and chaining FixedHypot() seems to give worse results.
		m = R_PointToDist2(0, 0, R_PointToDist2(0, 0, slope->normal.x, slope->normal.y), slope->normal.z);

		// Invert normal if it's facing down.
		if (slope->normal.z < 0)
			m = -m;

		FV3_Divide(&slope->normal, m);

		// Get direction vector
		m = FixedHypot(slope->normal.x, slope->normal.y);
		slope->d.x = -FixedDiv(slope->normal.x, m);
		slope->d.y = -FixedDiv(slope->normal.y, m);

		// Z delta
		slope->zdelta = FixedDiv(m, slope->normal.z);

		// Get angles
		slope->xydirection = R_PointToAngle2(0, 0, slope->d.x, slope->d.y)+ANGLE_180;
		slope->zangle = InvAngle(R_PointToAngle2(0, 0, FRACUNIT, slope->zdelta));
	}
}

/// Recalculate dynamic slopes.
void T_DynamicSlopeLine (dynplanethink_t* th)
{
	pslope_t* slope = th->slope;
	line_t* srcline = th->sourceline;

	fixed_t zdelta;

	switch(th->type) {
	case DP_FRONTFLOOR:
		zdelta = srcline->backsector->floorheight - srcline->frontsector->floorheight;
		slope->o.z = srcline->frontsector->floorheight;
		break;

	case DP_FRONTCEIL:
		zdelta = srcline->backsector->ceilingheight - srcline->frontsector->ceilingheight;
		slope->o.z = srcline->frontsector->ceilingheight;
		break;

	case DP_BACKFLOOR:
		zdelta = srcline->frontsector->floorheight - srcline->backsector->floorheight;
		slope->o.z = srcline->backsector->floorheight;
		break;

	case DP_BACKCEIL:
		zdelta = srcline->frontsector->ceilingheight - srcline->backsector->ceilingheight;
		slope->o.z = srcline->backsector->ceilingheight;
		break;

	default:
		return;
	}

	if (slope->zdelta != FixedDiv(zdelta, th->extent)) {
		slope->zdelta = FixedDiv(zdelta, th->extent);
		slope->zangle = R_PointToAngle2(0, 0, th->extent, -zdelta);
		P_CalculateSlopeNormal(slope);
	}
}

/// Mapthing-defined
void T_DynamicSlopeVert (dynplanethink_t* th)
{
	pslope_t* slope = th->slope;

	size_t i;
	INT32 l;

	for (i = 0; i < 3; i++) {
		l = P_FindSpecialLineFromTag(799, th->tags[i], -1);
		if (l != -1) {
			th->vex[i].z = lines[l].frontsector->floorheight;
		}
		else
			th->vex[i].z = 0;
	}

	ReconfigureViaVertexes(slope, th->vex[0], th->vex[1], th->vex[2]);
}

static inline void P_AddDynSlopeThinker (pslope_t* slope, dynplanetype_t type, line_t* sourceline, fixed_t extent, const INT16 tags[3], const vector3_t vx[3])
{
	dynplanethink_t* th = Z_Calloc(sizeof (*th), PU_LEVSPEC, NULL);
	switch (type)
	{
	case DP_VERTEX:
		th->thinker.function.acp1 = (actionf_p1)T_DynamicSlopeVert;
		memcpy(th->tags, tags, sizeof(th->tags));
		memcpy(th->vex, vx, sizeof(th->vex));
		break;
	default:
		th->thinker.function.acp1 = (actionf_p1)T_DynamicSlopeLine;
		th->sourceline = sourceline;
		th->extent = extent;
	}

	th->slope = slope;
	th->type = type;

	P_AddThinker(THINK_DYNSLOPE, &th->thinker);
}


/// Create a new slope and add it to the slope list.
static inline pslope_t* Slope_Add (const UINT8 flags)
{
	pslope_t *ret = Z_Calloc(sizeof(pslope_t), PU_LEVEL, NULL);
	ret->flags = flags;

	ret->next = slopelist;
	slopelist = ret;

	slopecount++;
	ret->id = slopecount;

	return ret;
}

/// Alocates and fill the contents of a slope structure.
static pslope_t *MakeViaVectors(const vector3_t *o, const vector2_t *d,
                             const fixed_t zdelta, UINT8 flags)
{
	pslope_t *ret = Slope_Add(flags);

	FV3_Copy(&ret->o, o);
	FV2_Copy(&ret->d, d);

	ret->zdelta = zdelta;

	ret->flags = flags;

	return ret;
}

/// Get furthest perpendicular distance from all vertexes in a sector for a given line.
static fixed_t GetExtent(sector_t *sector, line_t *line)
{
	// ZDoom code reference: v3float_t = vertex_t
	fixed_t fardist = -FRACUNIT;
	size_t i;

	// Find furthest vertex from the reference line. It, along with the two ends
	// of the line, will define the plane.
	for(i = 0; i < sector->linecount; i++)
	{
		line_t *li = sector->lines[i];
		vertex_t tempv;
		fixed_t dist;

		// Don't compare to the slope line.
		if(li == line)
			continue;

		P_ClosestPointOnLine(li->v1->x, li->v1->y, line, &tempv);
		dist = R_PointToDist2(tempv.x, tempv.y, li->v1->x, li->v1->y);
		if(dist > fardist)
			fardist = dist;

		// Okay, maybe do it for v2 as well?
		P_ClosestPointOnLine(li->v2->x, li->v2->y, line, &tempv);
		dist = R_PointToDist2(tempv.x, tempv.y, li->v2->x, li->v2->y);
		if(dist > fardist)
			fardist = dist;
	}

	return fardist;
}

/// Creates one or more slopes based on the given line type and front/back sectors.
static void line_SpawnViaLine(const int linenum, const boolean spawnthinker)
{
	// With dynamic slopes, it's fine to just leave this function as normal,
	// because checking to see if a slope had changed will waste more memory than
	// if the slope was just updated when called
	line_t *line = lines + linenum;
	pslope_t *fslope = NULL, *cslope = NULL;
	vector3_t origin, point;
	vector2_t direction;
	fixed_t nx, ny, dz, extent;

	boolean frontfloor = line->args[0] == TMS_FRONT;
	boolean backfloor = line->args[0] == TMS_BACK;
	boolean frontceil = line->args[1] == TMS_FRONT;
	boolean backceil = line->args[1] == TMS_BACK;
	UINT8 flags = 0; // Slope flags
	if (line->args[2] & TMSL_NOPHYSICS)
		flags |= SL_NOPHYSICS;
	if (line->args[2] & TMSL_DYNAMIC)
		flags |= SL_DYNAMIC;

	if(!frontfloor && !backfloor && !frontceil && !backceil)
	{
		CONS_Printf("line_SpawnViaLine: Slope special with nothing to do.\n");
		return;
	}

	if(!line->frontsector || !line->backsector)
	{
		CONS_Debug(DBG_SETUP, "line_SpawnViaLine: Slope special used on a line without two sides. (line number %i)\n", linenum);
		return;
	}

	{
		fixed_t len = R_PointToDist2(0, 0, line->dx, line->dy);
		nx = FixedDiv(line->dy, len);
		ny = -FixedDiv(line->dx, len);
	}

	// Set origin to line's center.
	origin.x = line->v1->x + (line->v2->x - line->v1->x)/2;
	origin.y = line->v1->y + (line->v2->y - line->v1->y)/2;

	// For FOF slopes, make a special function to copy to the xy origin & direction relative to the position of the FOF on the map!
	if(frontfloor || frontceil)
	{
		line->frontsector->hasslope = true; // Tell the software renderer that we're sloped

		origin.z = line->backsector->floorheight;
		direction.x = nx;
		direction.y = ny;

		extent = GetExtent(line->frontsector, line);

		if(extent < 0)
		{
			CONS_Printf("line_SpawnViaLine failed to get frontsector extent on line number %i\n", linenum);
			return;
		}

		// reposition the origin according to the extent
		point.x = origin.x + FixedMul(direction.x, extent);
		point.y = origin.y + FixedMul(direction.y, extent);
		direction.x = -direction.x;
		direction.y = -direction.y;

		// TODO: We take origin and point 's xy values and translate them to the center of an FOF!

		if(frontfloor)
		{
			point.z = line->frontsector->floorheight; // Startz
			dz = FixedDiv(origin.z - point.z, extent); // Destinationz

			// In P_SpawnSlopeLine the origin is the centerpoint of the sourcelinedef

			fslope = line->frontsector->f_slope =
            MakeViaVectors(&point, &direction, dz, flags);

			// Now remember that f_slope IS a vector
			// fslope->o = origin      3D point 1 of the vector
			// fslope->d = destination 3D point 2 of the vector
			// fslope->normal is a 3D line perpendicular to the 3D vector

			fslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			fslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(fslope);

			if (spawnthinker && (flags & SL_DYNAMIC))
				P_AddDynSlopeThinker(fslope, DP_FRONTFLOOR, line, extent, NULL, NULL);
		}
		if(frontceil)
		{
			origin.z = line->backsector->ceilingheight;
			point.z = line->frontsector->ceilingheight;
			dz = FixedDiv(origin.z - point.z, extent);

			cslope = line->frontsector->c_slope =
            MakeViaVectors(&point, &direction, dz, flags);

			cslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			cslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(cslope);

			if (spawnthinker && (flags & SL_DYNAMIC))
				P_AddDynSlopeThinker(cslope, DP_FRONTCEIL, line, extent, NULL, NULL);
		}
	}
	if(backfloor || backceil)
	{
		line->backsector->hasslope = true; // Tell the software renderer that we're sloped

		origin.z = line->frontsector->floorheight;
		// Backsector
		direction.x = -nx;
		direction.y = -ny;

		extent = GetExtent(line->backsector, line);

		if(extent < 0)
		{
			CONS_Printf("line_SpawnViaLine failed to get backsector extent on line number %i\n", linenum);
			return;
		}

		// reposition the origin according to the extent
		point.x = origin.x + FixedMul(direction.x, extent);
		point.y = origin.y + FixedMul(direction.y, extent);
		direction.x = -direction.x;
		direction.y = -direction.y;

		if(backfloor)
		{
			point.z = line->backsector->floorheight;
			dz = FixedDiv(origin.z - point.z, extent);

			fslope = line->backsector->f_slope =
            MakeViaVectors(&point, &direction, dz, flags);

			fslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			fslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(fslope);

			if (spawnthinker && (flags & SL_DYNAMIC))
				P_AddDynSlopeThinker(fslope, DP_BACKFLOOR, line, extent, NULL, NULL);
		}
		if(backceil)
		{
			origin.z = line->frontsector->ceilingheight;
			point.z = line->backsector->ceilingheight;
			dz = FixedDiv(origin.z - point.z, extent);

			cslope = line->backsector->c_slope =
            MakeViaVectors(&point, &direction, dz, flags);

			cslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			cslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(cslope);

			if (spawnthinker && (flags & SL_DYNAMIC))
				P_AddDynSlopeThinker(cslope, DP_BACKCEIL, line, extent, NULL, NULL);
		}
	}

	if(!line->tag)
		return;
}

/// Creates a new slope from three mapthings with the specified IDs
static pslope_t *MakeViaMapthings(INT16 tag1, INT16 tag2, INT16 tag3, UINT8 flags, const boolean spawnthinker)
{
	size_t i;
	mapthing_t* mt = mapthings;
	mapthing_t* vertices[3] = {0};
	INT16 tags[3] = {tag1, tag2, tag3};

	vector3_t vx[3];
	pslope_t* ret = Slope_Add(flags);

	// And... look for the vertices in question.
	for (i = 0; i < nummapthings; i++, mt++) {
		if (mt->type != 750) // Haha, I'm hijacking the old Chaos Spawn thingtype for something!
			continue;

		if (!vertices[0] && mt->tag == tag1)
			vertices[0] = mt;
		else if (!vertices[1] && mt->tag == tag2)
			vertices[1] = mt;
		else if (!vertices[2] && mt->tag == tag3)
			vertices[2] = mt;
	}

	// Now set heights for each vertex, because they haven't been set yet
	for (i = 0; i < 3; i++) {
		mt = vertices[i];
		if (!mt) // If a vertex wasn't found, it's game over. There's nothing you can do to recover (except maybe try and kill the slope instead - TODO?)
			I_Error("MakeViaMapthings: Slope vertex %s (for linedef tag %d) not found!", sizeu1(i), tag1);
		vx[i].x = mt->x << FRACBITS;
		vx[i].y = mt->y << FRACBITS;
		vx[i].z = mt->z << FRACBITS;
		if (!mt->extrainfo)
			vx[i].z += R_PointInSubsector(vx[i].x, vx[i].y)->sector->floorheight;
	}

	ReconfigureViaVertexes(ret, vx[0], vx[1], vx[2]);

	if (spawnthinker && (flags & SL_DYNAMIC))
		P_AddDynSlopeThinker(ret, DP_VERTEX, NULL, 0, tags, vx);

	return ret;
}

/// Create vertex based slopes using tagged mapthings.
static void line_SpawnViaMapthingVertexes(const int linenum, const boolean spawnthinker)
{
	line_t *line = lines + linenum;
	side_t *side;
	pslope_t **slopetoset;
	UINT16 tag1 = line->args[1];
	UINT16 tag2 = line->args[2];
	UINT16 tag3 = line->args[3];
	UINT8 flags = 0; // Slope flags
	if (line->args[4] & TMSL_NOPHYSICS)
		flags |= SL_NOPHYSICS;
	if (line->args[4] & TMSL_DYNAMIC)
		flags |= SL_DYNAMIC;

	switch(line->args[0])
	{
	case TMSP_FRONTFLOOR:
		slopetoset = &line->frontsector->f_slope;
		side = &sides[line->sidenum[0]];
		break;
	case TMSP_FRONTCEILING:
		slopetoset = &line->frontsector->c_slope;
		side = &sides[line->sidenum[0]];
		break;
	case TMSP_BACKFLOOR:
		slopetoset = &line->backsector->f_slope;
		side = &sides[line->sidenum[1]];
		break;
	case TMSP_BACKCEILING:
		slopetoset = &line->backsector->c_slope;
		side = &sides[line->sidenum[1]];
	default:
		return;
	}

	*slopetoset = MakeViaMapthings(tag1, tag2, tag3, flags, spawnthinker);

	side->sector->hasslope = true;
}

/// Spawn textmap vertex slopes.
static void SpawnVertexSlopes(void)
{
	line_t *l1, *l2;
	sector_t* sc;
	vertex_t *v1, *v2, *v3;
	size_t i;
	for (i = 0, sc = sectors; i < numsectors; i++, sc++)
	{
		// The vertex slopes only work for 3-vertex sectors (and thus 3-sided sectors).
		if (sc->linecount != 3)
			continue;

		l1 = sc->lines[0];
		l2 = sc->lines[1];

		// Determine the vertexes.
		v1 = l1->v1;
		v2 = l1->v2;
		if ((l2->v1 != v1) && (l2->v1 != v2))
			v3 = l2->v1;
		else
			v3 = l2->v2;

		if (v1->floorzset || v2->floorzset || v3->floorzset)
		{
			vector3_t vtx[3] = {
				{v1->x, v1->y, v1->floorzset ? v1->floorz : sc->floorheight},
				{v2->x, v2->y, v2->floorzset ? v2->floorz : sc->floorheight},
				{v3->x, v3->y, v3->floorzset ? v3->floorz : sc->floorheight}};
			pslope_t *slop = Slope_Add(0);
			sc->f_slope = slop;
			sc->hasslope = true;
			ReconfigureViaVertexes(slop, vtx[0], vtx[1], vtx[2]);
		}

		if (v1->ceilingzset || v2->ceilingzset || v3->ceilingzset)
		{
			vector3_t vtx[3] = {
				{v1->x, v1->y, v1->ceilingzset ? v1->ceilingz : sc->ceilingheight},
				{v2->x, v2->y, v2->ceilingzset ? v2->ceilingz : sc->ceilingheight},
				{v3->x, v3->y, v3->ceilingzset ? v3->ceilingz : sc->ceilingheight}};
			pslope_t *slop = Slope_Add(0);
			sc->c_slope = slop;
			sc->hasslope = true;
			ReconfigureViaVertexes(slop, vtx[0], vtx[1], vtx[2]);
		}
	}
}

static boolean P_SetSlopeFromTag(sector_t *sec, INT32 tag, boolean ceiling)
{
	INT32 i;
	pslope_t **secslope = ceiling ? &sec->c_slope : &sec->f_slope;

	if (!tag || *secslope)
		return false;

	for (i = -1; (i = P_FindSectorFromTag(tag, i)) >= 0;)
	{
		pslope_t *srcslope = ceiling ? sectors[i].c_slope : sectors[i].f_slope;
		if (srcslope)
		{
			*secslope = srcslope;
			return true;
		}
	}
	return false;
}

static boolean P_CopySlope(pslope_t **toslope, pslope_t *fromslope)
{
	if (*toslope || !fromslope)
		return true;

	*toslope = fromslope;
	return true;
}

static void P_UpdateHasSlope(sector_t *sec)
{
	size_t i;

	sec->hasslope = true;

	// if this is an FOF control sector, make sure any target sectors also are marked as having slopes
	if (sec->numattached)
		for (i = 0; i < sec->numattached; i++)
			sectors[sec->attached[i]].hasslope = true;
}

//
// P_CopySectorSlope
//
// Searches through tagged sectors and copies
//
void P_CopySectorSlope(line_t *line)
{
	sector_t *fsec = line->frontsector;
	sector_t *bsec = line->backsector;
	boolean setfront = false;
	boolean setback = false;

	setfront |= P_SetSlopeFromTag(fsec, line->args[0], false);
	setfront |= P_SetSlopeFromTag(fsec, line->args[1], true);
	if (bsec)
	{
		setback |= P_SetSlopeFromTag(bsec, line->args[2], false);
		setback |= P_SetSlopeFromTag(bsec, line->args[3], true);

		if (line->args[4] & TMSC_FRONTTOBACKFLOOR)
			setback |= P_CopySlope(&bsec->f_slope, fsec->f_slope);
		if (line->args[4] & TMSC_BACKTOFRONTFLOOR)
			setfront |= P_CopySlope(&fsec->f_slope, bsec->f_slope);
		if (line->args[4] & TMSC_FRONTTOBACKCEILING)
			setback |= P_CopySlope(&bsec->c_slope, fsec->c_slope);
		if (line->args[4] & TMSC_BACKTOFRONTCEILING)
			setfront |= P_CopySlope(&fsec->c_slope, bsec->c_slope);
	}

	if (setfront)
		P_UpdateHasSlope(fsec);
	if (setback)
		P_UpdateHasSlope(bsec);

	line->special = 0; // Linedef was use to set slopes, it finished its job, so now make it a normal linedef
}

//
// P_SlopeById
//
// Looks in the slope list for a slope with a specified ID. Mostly useful for netgame sync
//
pslope_t *P_SlopeById(UINT16 id)
{
	pslope_t *ret;
	for (ret = slopelist; ret && ret->id != id; ret = ret->next);
	return ret;
}

/// Initializes and reads the slopes from the map data.
void P_SpawnSlopes(const boolean fromsave) {
	size_t i;

	slopelist = NULL;
	slopecount = 0;

	/// Generates vertex slopes.
	SpawnVertexSlopes();

	/// Generates line special-defined slopes.
	for (i = 0; i < numlines; i++)
	{
		switch (lines[i].special)
		{
			case 700:
				line_SpawnViaLine(i, !fromsave);
				break;

			case 704:
				line_SpawnViaMapthingVertexes(i, !fromsave);
				break;

			default:
				break;
		}
	}

	/// Copies slopes from tagged sectors via line specials.
	/// \note Doesn't actually copy, but instead they share the same pointers.
	for (i = 0; i < numlines; i++)
		switch (lines[i].special)
		{
			case 720:
				P_CopySectorSlope(&lines[i]);
			default:
				break;
		}
}

// ============================================================================
//
// Various utilities related to slopes
//

// Returns the height of the sloped plane at (x, y) as a fixed_t
fixed_t P_GetSlopeZAt(const pslope_t *slope, fixed_t x, fixed_t y)
{
	fixed_t dist = FixedMul(x - slope->o.x, slope->d.x) +
	               FixedMul(y - slope->o.y, slope->d.y);

	return slope->o.z + FixedMul(dist, slope->zdelta);
}

// Like P_GetSlopeZAt but falls back to z if slope is NULL
fixed_t P_GetZAt(const pslope_t *slope, fixed_t x, fixed_t y, fixed_t z)
{
	return slope ? P_GetSlopeZAt(slope, x, y) : z;
}

// Returns the height of the sector floor at (x, y)
fixed_t P_GetSectorFloorZAt(const sector_t *sector, fixed_t x, fixed_t y)
{
	return sector->f_slope ? P_GetSlopeZAt(sector->f_slope, x, y) : sector->floorheight;
}

// Returns the height of the sector ceiling at (x, y)
fixed_t P_GetSectorCeilingZAt(const sector_t *sector, fixed_t x, fixed_t y)
{
	return sector->c_slope ? P_GetSlopeZAt(sector->c_slope, x, y) : sector->ceilingheight;
}

// Returns the height of the FOF top at (x, y)
fixed_t P_GetFFloorTopZAt(const ffloor_t *ffloor, fixed_t x, fixed_t y)
{
	return *ffloor->t_slope ? P_GetSlopeZAt(*ffloor->t_slope, x, y) : *ffloor->topheight;
}

// Returns the height of the FOF bottom  at (x, y)
fixed_t P_GetFFloorBottomZAt(const ffloor_t *ffloor, fixed_t x, fixed_t y)
{
	return *ffloor->b_slope ? P_GetSlopeZAt(*ffloor->b_slope, x, y) : *ffloor->bottomheight;
}

// Returns the height of the light list at (x, y)
fixed_t P_GetLightZAt(const lightlist_t *light, fixed_t x, fixed_t y)
{
	return light->slope ? P_GetSlopeZAt(light->slope, x, y) : light->height;
}


//
// P_QuantizeMomentumToSlope
//
// When given a vector, rotates it and aligns it to a slope
void P_QuantizeMomentumToSlope(vector3_t *momentum, pslope_t *slope)
{
	vector3_t axis; // Fuck you, C90.

	if (slope->flags & SL_NOPHYSICS)
		return; // No physics, no quantizing.

	axis.x = -slope->d.y;
	axis.y = slope->d.x;
	axis.z = 0;

	FV3_Rotate(momentum, &axis, slope->zangle >> ANGLETOFINESHIFT);
}

//
// P_ReverseQuantizeMomentumToSlope
//
// When given a vector, rotates and aligns it to a flat surface (from being relative to a given slope)
void P_ReverseQuantizeMomentumToSlope(vector3_t *momentum, pslope_t *slope)
{
	slope->zangle = InvAngle(slope->zangle);
	P_QuantizeMomentumToSlope(momentum, slope);
	slope->zangle = InvAngle(slope->zangle);
}

//
// P_SlopeLaunch
//
// Handles slope ejection for objects
void P_SlopeLaunch(mobj_t *mo)
{
	if (!(mo->standingslope->flags & SL_NOPHYSICS) // If there's physics, time for launching.
		&& (mo->standingslope->normal.x != 0
		||  mo->standingslope->normal.y != 0))
	{
		// Double the pre-rotation Z, then halve the post-rotation Z. This reduces the
		// vertical launch given from slopes while increasing the horizontal launch
		// given. Good for SRB2's gravity and horizontal speeds.
		vector3_t slopemom;
		slopemom.x = mo->momx;
		slopemom.y = mo->momy;
		slopemom.z = mo->momz*2;
		P_QuantizeMomentumToSlope(&slopemom, mo->standingslope);

		mo->momx = slopemom.x;
		mo->momy = slopemom.y;
		mo->momz = slopemom.z/2;
	}

	//CONS_Printf("Launched off of slope.\n");
	mo->standingslope = NULL;

	if (mo->player)
		mo->player->powers[pw_justlaunched] = 1;
}

//
// P_GetWallTransferMomZ
//
// It would be nice to have a single function that does everything necessary for slope-to-wall transfer.
// However, it needs to be seperated out in P_XYMovement to take into account momentum before and after hitting the wall.
// This just performs the necessary calculations for getting the base vertical momentum; the horizontal is already reasonably calculated by P_SlideMove.
fixed_t P_GetWallTransferMomZ(mobj_t *mo, pslope_t *slope)
{
	vector3_t slopemom, axis;
	angle_t ang;

	if (mo->standingslope->flags & SL_NOPHYSICS)
		return 0;

	// If there's physics, time for launching.
	// Doesn't kill the vertical momentum as much as P_SlopeLaunch does.
	ang = slope->zangle + ANG15*((slope->zangle > 0) ? 1 : -1);
	if (ang > ANGLE_90 && ang < ANGLE_180)
		ang = ((slope->zangle > 0) ? ANGLE_90 : InvAngle(ANGLE_90)); // hard cap of directly upwards

	slopemom.x = mo->momx;
	slopemom.y = mo->momy;
	slopemom.z = 3*(mo->momz/2);

	axis.x = -slope->d.y;
	axis.y = slope->d.x;
	axis.z = 0;

	FV3_Rotate(&slopemom, &axis, ang >> ANGLETOFINESHIFT);

	return 2*(slopemom.z/3);
}

// Function to help handle landing on slopes
void P_HandleSlopeLanding(mobj_t *thing, pslope_t *slope)
{
	vector3_t mom; // Ditto.
	if (slope->flags & SL_NOPHYSICS || (slope->normal.x == 0 && slope->normal.y == 0)) { // No physics, no need to make anything complicated.
		if (P_MobjFlip(thing)*(thing->momz) < 0) // falling, land on slope
		{
			thing->standingslope = slope;
			if (!thing->player || !(thing->player->pflags & PF_BOUNCING))
				thing->momz = -P_MobjFlip(thing);
		}
		return;
	}

	mom.x = thing->momx;
	mom.y = thing->momy;
	mom.z = thing->momz*2;

	P_ReverseQuantizeMomentumToSlope(&mom, slope);

	if (P_MobjFlip(thing)*mom.z < 0) { // falling, land on slope
		thing->momx = mom.x;
		thing->momy = mom.y;
		thing->standingslope = slope;
		if (!thing->player || !(thing->player->pflags & PF_BOUNCING))
			thing->momz = -P_MobjFlip(thing);
	}
}

// https://yourlogicalfallacyis.com/slippery-slope
// Handles sliding down slopes, like if they were made of butter :)
void P_ButteredSlope(mobj_t *mo)
{
	fixed_t thrust;

	if (!mo->standingslope)
		return;

	if (mo->standingslope->flags & SL_NOPHYSICS)
		return; // No physics, no butter.

	if (mo->flags & (MF_NOCLIPHEIGHT|MF_NOGRAVITY))
		return; // don't slide down slopes if you can't touch them or you're not affected by gravity

	if (mo->player) {
		if (abs(mo->standingslope->zdelta) < FRACUNIT/4 && !(mo->player->pflags & PF_SPINNING))
			return; // Don't slide on non-steep slopes unless spinning

		if (abs(mo->standingslope->zdelta) < FRACUNIT/2 && !(mo->player->rmomx || mo->player->rmomy))
			return; // Allow the player to stand still on slopes below a certain steepness
	}

	thrust = FINESINE(mo->standingslope->zangle>>ANGLETOFINESHIFT) * 3 / 2 * (mo->eflags & MFE_VERTICALFLIP ? 1 : -1);

	if (mo->player && (mo->player->pflags & PF_SPINNING)) {
		fixed_t mult = 0;
		if (mo->momx || mo->momy) {
			angle_t angle = R_PointToAngle2(0, 0, mo->momx, mo->momy) - mo->standingslope->xydirection;

			if (P_MobjFlip(mo) * mo->standingslope->zdelta < 0)
				angle ^= ANGLE_180;

			mult = FINECOSINE(angle >> ANGLETOFINESHIFT);
		}

		thrust = FixedMul(thrust, FRACUNIT*2/3 + mult/8);
	}

	if (mo->momx || mo->momy) // Slightly increase thrust based on the object's speed
		thrust = FixedMul(thrust, FRACUNIT+P_AproxDistance(mo->momx, mo->momy)/16);
	// This makes it harder to zigzag up steep slopes, as well as allows greater top speed when rolling down

	// Let's get the gravity strength for the object...
	thrust = FixedMul(thrust, abs(P_GetMobjGravity(mo)));

	// ... and its friction against the ground for good measure (divided by original friction to keep behaviour for normal slopes the same).
	thrust = FixedMul(thrust, FixedDiv(mo->friction, ORIG_FRICTION));

	P_Thrust(mo, mo->standingslope->xydirection, thrust);
}
