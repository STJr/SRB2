// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2004 Stephen McGranahan
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//      Slopes
//      SoM created 05/10/09
//      ZDoom + Eternity Engine Slopes, ported and enhanced by Kalaron
//
//-----------------------------------------------------------------------------


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

#ifdef ESLOPE

static pslope_t *slopelist = NULL;
static UINT16 slopecount = 0;

// Calculate line normal
static void P_CalculateSlopeNormal(pslope_t *slope) {
	slope->normal.z = FINECOSINE(slope->zangle>>ANGLETOFINESHIFT);
	slope->normal.x = -FixedMul(FINESINE(slope->zangle>>ANGLETOFINESHIFT), slope->d.x);
	slope->normal.y = -FixedMul(FINESINE(slope->zangle>>ANGLETOFINESHIFT), slope->d.y);
}

// With a vertex slope that has its vertices set, configure relevant slope info
static void P_ReconfigureVertexSlope(pslope_t *slope)
{
	vector3_t vec1, vec2;

	// Set slope normal
	vec1.x = (slope->vertices[1]->x - slope->vertices[0]->x) << FRACBITS;
	vec1.y = (slope->vertices[1]->y - slope->vertices[0]->y) << FRACBITS;
	vec1.z = (slope->vertices[1]->z - slope->vertices[0]->z) << FRACBITS;

	vec2.x = (slope->vertices[2]->x - slope->vertices[0]->x) << FRACBITS;
	vec2.y = (slope->vertices[2]->y - slope->vertices[0]->y) << FRACBITS;
	vec2.z = (slope->vertices[2]->z - slope->vertices[0]->z) << FRACBITS;

	// ugggggggh fixed-point maaaaaaath
	slope->extent = max(
		max(max(abs(vec1.x), abs(vec1.y)), abs(vec1.z)),
		max(max(abs(vec2.x), abs(vec2.y)), abs(vec2.z))
	) >> (FRACBITS+5);
	vec1.x /= slope->extent;
	vec1.y /= slope->extent;
	vec1.z /= slope->extent;
	vec2.x /= slope->extent;
	vec2.y /= slope->extent;
	vec2.z /= slope->extent;

	FV3_Cross(&vec1, &vec2, &slope->normal);

	slope->extent = R_PointToDist2(0, 0, R_PointToDist2(0, 0, slope->normal.x, slope->normal.y), slope->normal.z);
	if (slope->normal.z < 0)
		slope->extent = -slope->extent;

	slope->normal.x = FixedDiv(slope->normal.x, slope->extent);
	slope->normal.y = FixedDiv(slope->normal.y, slope->extent);
	slope->normal.z = FixedDiv(slope->normal.z, slope->extent);

	// Set origin
	slope->o.x = slope->vertices[0]->x << FRACBITS;
	slope->o.y = slope->vertices[0]->y << FRACBITS;
	slope->o.z = slope->vertices[0]->z << FRACBITS;

	if (slope->normal.x == 0 && slope->normal.y == 0) { // Set some defaults for a non-sloped "slope"
		slope->zangle = slope->xydirection = 0;
		slope->zdelta = slope->d.x = slope->d.y = 0;
	} else {
		// Get direction vector
		slope->extent = R_PointToDist2(0, 0, slope->normal.x, slope->normal.y);
		slope->d.x = -FixedDiv(slope->normal.x, slope->extent);
		slope->d.y = -FixedDiv(slope->normal.y, slope->extent);

		// Z delta
		slope->zdelta = FixedDiv(slope->extent, slope->normal.z);

		// Get angles
		slope->xydirection = R_PointToAngle2(0, 0, slope->d.x, slope->d.y)+ANGLE_180;
		slope->zangle = -R_PointToAngle2(0, 0, FRACUNIT, slope->zdelta);
	}
}

// Recalculate dynamic slopes
void P_RunDynamicSlopes(void) {
	pslope_t *slope;

	for (slope = slopelist; slope; slope = slope->next) {
		fixed_t zdelta;

		if (slope->flags & SL_NODYNAMIC)
			continue;

		switch(slope->refpos) {
		case 1: // front floor
			zdelta = slope->sourceline->backsector->floorheight - slope->sourceline->frontsector->floorheight;
			slope->o.z = slope->sourceline->frontsector->floorheight;
			break;
		case 2: // front ceiling
			zdelta = slope->sourceline->backsector->ceilingheight - slope->sourceline->frontsector->ceilingheight;
			slope->o.z = slope->sourceline->frontsector->ceilingheight;
			break;
		case 3: // back floor
			zdelta = slope->sourceline->frontsector->floorheight - slope->sourceline->backsector->floorheight;
			slope->o.z = slope->sourceline->backsector->floorheight;
			break;
		case 4: // back ceiling
			zdelta = slope->sourceline->frontsector->ceilingheight - slope->sourceline->backsector->ceilingheight;
			slope->o.z = slope->sourceline->backsector->ceilingheight;
			break;
		case 5: // vertices
			{
				mapthing_t *mt;
				size_t i;
				INT32 l;
				line_t *line;

				for (i = 0; i < 3; i++) {
					mt = slope->vertices[i];
					l = P_FindSpecialLineFromTag(799, mt->angle, -1);
					if (l != -1) {
						line = &lines[l];
						mt->z = line->frontsector->floorheight >> FRACBITS;
					}
				}

				P_ReconfigureVertexSlope(slope);
			}
			continue; // TODO

		default:
			I_Error("P_RunDynamicSlopes: slope has invalid type!");
		}

		if (slope->zdelta != FixedDiv(zdelta, slope->extent)) {
			slope->zdelta = FixedDiv(zdelta, slope->extent);
			slope->zangle = R_PointToAngle2(0, 0, slope->extent, -zdelta);
			P_CalculateSlopeNormal(slope);
		}
	}
}

//
// P_MakeSlope
//
// Alocates and fill the contents of a slope structure.
//
static pslope_t *P_MakeSlope(const vector3_t *o, const vector2_t *d,
                             const fixed_t zdelta, UINT8 flags)
{
	pslope_t *ret = Z_Malloc(sizeof(pslope_t), PU_LEVEL, NULL);
	memset(ret, 0, sizeof(*ret));

	ret->o.x = o->x;
	ret->o.y = o->y;
	ret->o.z = o->z;

	ret->d.x = d->x;
	ret->d.y = d->y;

	ret->zdelta = zdelta;

	ret->flags = flags;

	// Add to the slope list
	ret->next = slopelist;
	slopelist = ret;

	slopecount++;
	ret->id = slopecount;

	return ret;
}

//
// P_GetExtent
//
// Returns the distance to the first line within the sector that
// is intersected by a line parallel to the plane normal with the point (ox, oy)
//
static fixed_t P_GetExtent(sector_t *sector, line_t *line)
{
	// ZDoom code reference: v3float_t = vertex_t
	fixed_t fardist = -FRACUNIT;
	size_t i;

	// Find furthest vertex from the reference line. It, along with the two ends
	// of the line, will define the plane.
	// SRB2CBTODO: Use a formula to get the slope to slide objects depending on how steep
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


//
// P_SpawnSlope_Line
//
// Creates one or more slopes based on the given line type and front/back
// sectors.
// Kalaron: Check if dynamic slopes need recalculation
//
void P_SpawnSlope_Line(int linenum)
{
	// With dynamic slopes, it's fine to just leave this function as normal,
	// because checking to see if a slope had changed will waste more memory than
	// if the slope was just updated when called
	line_t *line = lines + linenum;
	INT16 special = line->special;
	pslope_t *fslope = NULL, *cslope = NULL;
	vector3_t origin, point;
	vector2_t direction;
	fixed_t nx, ny, dz, extent;

	boolean frontfloor = (special == 700 || special == 702 || special == 703);
	boolean backfloor  = (special == 710 || special == 712 || special == 713);
	boolean frontceil  = (special == 701 || special == 702 || special == 713);
	boolean backceil   = (special == 711 || special == 712 || special == 703);

	UINT8 flags = 0; // Slope flags
	if (line->flags & ML_NOSONIC)
		flags |= SL_NOPHYSICS;
	if (line->flags & ML_NOTAILS)
		flags |= SL_NODYNAMIC;
	if (line->flags & ML_NOKNUX)
		flags |= SL_ANCHORVERTEX;

	if(!frontfloor && !backfloor && !frontceil && !backceil)
	{
		CONS_Printf("P_SpawnSlope_Line called with non-slope line special.\n");
		return;
	}

	if(!line->frontsector || !line->backsector)
	{
		CONS_Printf("P_SpawnSlope_Line used on a line without two sides.\n");
		return;
	}

	{
		fixed_t len = R_PointToDist2(0, 0, line->dx, line->dy);
		nx = FixedDiv(line->dy, len);
		ny = -FixedDiv(line->dx, len);
	}

	// SRB2CBTODO: Transform origin relative to the bounds of an individual FOF
	origin.x = line->v1->x + (line->v2->x - line->v1->x)/2;
	origin.y = line->v1->y + (line->v2->y - line->v1->y)/2;

	// For FOF slopes, make a special function to copy to the xy origin & direction relative to the position of the FOF on the map!
	if(frontfloor || frontceil)
	{
		line->frontsector->hasslope = true; // Tell the software renderer that we're sloped

		origin.z = line->backsector->floorheight;
		direction.x = nx;
		direction.y = ny;

		extent = P_GetExtent(line->frontsector, line);

		if(extent < 0)
		{
			CONS_Printf("P_SpawnSlope_Line failed to get frontsector extent on line number %i\n", linenum);
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
			fixed_t highest, lowest;
			size_t l;
			point.z = line->frontsector->floorheight; // Startz
			dz = FixedDiv(origin.z - point.z, extent); // Destinationz

			// In P_SpawnSlopeLine the origin is the centerpoint of the sourcelinedef

			fslope = line->frontsector->f_slope =
            P_MakeSlope(&point, &direction, dz, flags);

            // Set up some shit
            fslope->extent = extent;
            fslope->refpos = 1;

			// Now remember that f_slope IS a vector
			// fslope->o = origin      3D point 1 of the vector
			// fslope->d = destination 3D point 2 of the vector
			// fslope->normal is a 3D line perpendicular to the 3D vector

			// Sync the linedata of the line that started this slope
			// SRB2CBTODO: Anything special for remote(control sector)-based slopes later?
			fslope->sourceline = line;

			// To find the real highz/lowz of a slope, you need to check all the vertexes
			// in the slope's sector with P_GetZAt to get the REAL lowz & highz
			// Although these slopes are set by floorheights the ANGLE is what a slope is,
			// so technically any slope can extend on forever (they are just bound by sectors)
			// *You can use sourceline as a reference to see if two slopes really are the same

			// Default points for high and low
			highest = point.z > origin.z ? point.z : origin.z;
			lowest = point.z < origin.z ? point.z : origin.z;

			// Now check to see what the REAL high and low points of the slope inside the sector
			// TODO: Is this really needed outside of FOFs? -Red

			for (l = 0; l < line->frontsector->linecount; l++)
			{
				fixed_t height = P_GetZAt(line->frontsector->f_slope, line->frontsector->lines[l]->v1->x, line->frontsector->lines[l]->v1->y);

				if (height > highest)
					highest = height;

				if (height < lowest)
					lowest = height;
			}

			// Sets extra clipping data for the frontsector's slope
			fslope->highz = highest;
			fslope->lowz = lowest;

			fslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			fslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(fslope);
		}
		if(frontceil)
		{
			fixed_t highest, lowest;
			size_t l;
			origin.z = line->backsector->ceilingheight;
			point.z = line->frontsector->ceilingheight;
			dz = FixedDiv(origin.z - point.z, extent);

			cslope = line->frontsector->c_slope =
            P_MakeSlope(&point, &direction, dz, flags);

            // Set up some shit
            cslope->extent = extent;
            cslope->refpos = 2;

			// Sync the linedata of the line that started this slope
			// SRB2CBTODO: Anything special for remote(control sector)-based slopes later?
			cslope->sourceline = line;

			// Remember the way the slope is formed
			highest = point.z > origin.z ? point.z : origin.z;
			lowest = point.z < origin.z ? point.z : origin.z;

			for (l = 0; l < line->frontsector->linecount; l++)
			{
				fixed_t height = P_GetZAt(line->frontsector->c_slope, line->frontsector->lines[l]->v1->x, line->frontsector->lines[l]->v1->y);

				if (height > highest)
					highest = height;

				if (height < lowest)
					lowest = height;
			}

			// This line special sets extra clipping data for the frontsector's slope
			cslope->highz = highest;
			cslope->lowz = lowest;

			cslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			cslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(cslope);
		}
	}
	if(backfloor || backceil)
	{
		line->backsector->hasslope = true; // Tell the software renderer that we're sloped

		origin.z = line->frontsector->floorheight;
		// Backsector
		direction.x = -nx;
		direction.y = -ny;

		extent = P_GetExtent(line->backsector, line);

		if(extent < 0)
		{
			CONS_Printf("P_SpawnSlope_Line failed to get backsector extent on line number %i\n", linenum);
			return;
		}

		// reposition the origin according to the extent
		point.x = origin.x + FixedMul(direction.x, extent);
		point.y = origin.y + FixedMul(direction.y, extent);
		direction.x = -direction.x;
		direction.y = -direction.y;

		if(backfloor)
		{
			fixed_t highest, lowest;
			size_t l;
			point.z = line->backsector->floorheight;
			dz = FixedDiv(origin.z - point.z, extent);

			fslope = line->backsector->f_slope =
            P_MakeSlope(&point, &direction, dz, flags);

            // Set up some shit
            fslope->extent = extent;
            fslope->refpos = 3;

			// Sync the linedata of the line that started this slope
			// SRB2CBTODO: Anything special for remote(control sector)-based slopes later?
			fslope->sourceline = line;

			// Remember the way the slope is formed
			highest = point.z > origin.z ? point.z : origin.z;
			lowest = point.z < origin.z ? point.z : origin.z;

			for (l = 0; l < line->backsector->linecount; l++)
			{
				fixed_t height = P_GetZAt(line->backsector->f_slope, line->backsector->lines[l]->v1->x, line->backsector->lines[l]->v1->y);

				if (height > highest)
					highest = height;

				if (height < lowest)
					lowest = height;
			}

			// This line special sets extra clipping data for the frontsector's slope
			fslope->highz = highest;
			fslope->lowz = lowest;

			fslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			fslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(fslope);
		}
		if(backceil)
		{
			fixed_t highest, lowest;
			size_t l;
			origin.z = line->frontsector->ceilingheight;
			point.z = line->backsector->ceilingheight;
			dz = FixedDiv(origin.z - point.z, extent);

			cslope = line->backsector->c_slope =
            P_MakeSlope(&point, &direction, dz, flags);

            // Set up some shit
            cslope->extent = extent;
            cslope->refpos = 4;

			// Sync the linedata of the line that started this slope
			// SRB2CBTODO: Anything special for remote(control sector)-based slopes later?
			cslope->sourceline = line;

			// Remember the way the slope is formed
			highest = point.z > origin.z ? point.z : origin.z;
			lowest = point.z < origin.z ? point.z : origin.z;

			for (l = 0; l < line->backsector->linecount; l++)
			{
				fixed_t height = P_GetZAt(line->backsector->c_slope, line->backsector->lines[l]->v1->x, line->backsector->lines[l]->v1->y);

				if (height > highest)
					highest = height;

				if (height < lowest)
					lowest = height;
			}

			// This line special sets extra clipping data for the backsector's slope
			cslope->highz = highest;
			cslope->lowz = lowest;

			cslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			cslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(cslope);
		}
	}

	if(!line->tag)
		return;
}

//
// P_NewVertexSlope
//
// Creates a new slope from three vertices with the specified IDs
//
static pslope_t *P_NewVertexSlope(INT16 tag1, INT16 tag2, INT16 tag3, UINT8 flags)
{
	size_t i;
	mapthing_t *mt = mapthings;

	pslope_t *ret = Z_Malloc(sizeof(pslope_t), PU_LEVEL, NULL);
	memset(ret, 0, sizeof(*ret));

	// Start by setting flags
	ret->flags = flags;

	// Now set up the vertex list
	ret->vertices = Z_Malloc(3*sizeof(mapthing_t), PU_LEVEL, NULL);
	memset(ret->vertices, 0, 3*sizeof(mapthing_t));

	// And... look for the vertices in question.
	for (i = 0; i < nummapthings; i++, mt++) {
		if (mt->type != 750) // Haha, I'm hijacking the old Chaos Spawn thingtype for something!
			continue;

		if (!ret->vertices[0] && mt->angle == tag1)
			ret->vertices[0] = mt;
		else if (!ret->vertices[1] && mt->angle == tag2)
			ret->vertices[1] = mt;
		else if (!ret->vertices[2] && mt->angle == tag3)
			ret->vertices[2] = mt;
	}

	if (!ret->vertices[0])
		CONS_Printf("PANIC 0\n");
	if (!ret->vertices[1])
		CONS_Printf("PANIC 1\n");
	if (!ret->vertices[2])
		CONS_Printf("PANIC 2\n");

	// Now set heights for each vertex, because they haven't been set yet
	for (i = 0; i < 3; i++) {
		mt = ret->vertices[i];
		if (mt->extrainfo)
			mt->z = mt->options;
		else
			mt->z = (R_PointInSubsector(mt->x << FRACBITS, mt->y << FRACBITS)->sector->floorheight >> FRACBITS) + (mt->options >> ZSHIFT);
	}

	P_ReconfigureVertexSlope(ret);
	ret->refpos = 5;

	// Add to the slope list
	ret->next = slopelist;
	slopelist = ret;

	slopecount++;
	ret->id = slopecount;

	return ret;
}



//
// P_CopySectorSlope
//
// Searches through tagged sectors and copies
//
void P_CopySectorSlope(line_t *line)
{
   sector_t *fsec = line->frontsector;
   int i, special = line->special;

   // Check for copy linedefs
   for(i = -1; (i = P_FindSectorFromLineTag(line, i)) >= 0;)
   {
      sector_t *srcsec = sectors + i;

      if((special - 719) & 1 && !fsec->f_slope && srcsec->f_slope)
         fsec->f_slope = srcsec->f_slope; //P_CopySlope(srcsec->f_slope);
      if((special - 719) & 2 && !fsec->c_slope && srcsec->c_slope)
         fsec->c_slope = srcsec->c_slope; //P_CopySlope(srcsec->c_slope);
   }

   fsec->hasslope = true;

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

#ifdef SPRINGCLEAN
#include "byteptr.h"

#include "p_setup.h"
#include "p_local.h"

//==========================================================================
//
//	P_SetSlopesFromVertexHeights
//
//==========================================================================
void P_SetSlopesFromVertexHeights(lumpnum_t lumpnum)
{
	mapthing_t *mt;
	boolean vt_found = false;
	size_t i, j, k, l, q;

	//size_t i;
	//mapthing_t *mt;
	char *data;
	char *datastart;

	// SRB2CBTODO: WHAT IS (5 * sizeof (short))?! It = 10
	// anything else seems to make a map not load properly,
	// but this hard-coded value MUST have some reason for being what it is
	size_t snummapthings = W_LumpLength(lumpnum) / (5 * sizeof (short));
	mapthing_t *smapthings = Z_Calloc(snummapthings * sizeof (*smapthings), PU_LEVEL, NULL);
	fixed_t x, y;
	sector_t *sector;
	// Spawn axis points first so they are
	// at the front of the list for fast searching.
	data = datastart = W_CacheLumpNum(lumpnum, PU_LEVEL);
	mt = smapthings;
	for (i = 0; i < snummapthings; i++, mt++)
	{
		mt->x = READINT16(data);
		mt->y = READINT16(data);
		mt->angle = READINT16(data);
		mt->type = READINT16(data);
		mt->options = READINT16(data);
		// mt->z hasn't been set yet!
		//mt->extrainfo = (byte)(mt->type >> 12); // slope things are special, they have a bigger range of types

		//mt->type &= 4095; // SRB2CBTODO: WHAT IS THIS???? Mobj type limits?!!!!
		x = mt->x*FRACUNIT;
		y = mt->y*FRACUNIT;
		sector = R_PointInSubsector(x, y)->sector;
		// Z for objects
#ifdef ESLOPE
		if (sector->f_slope)
			mt->z = (short)(P_GetZAt(sector->f_slope, x, y)>>FRACBITS);
		else
#endif
			mt->z = (short)(sector->floorheight>>FRACBITS);

		mt->z = mt->z + (mt->options >> ZSHIFT);

		if (mt->type == THING_VertexFloorZ || mt->type == THING_VertexCeilingZ) // THING_VertexFloorZ
		{
			for(l = 0; l < numvertexes; l++)
			{
				if (vertexes[l].x == mt->x*FRACUNIT && vertexes[l].y == mt->y*FRACUNIT)
				{
					if (mt->type == THING_VertexFloorZ)
					{
						vertexes[l].z = mt->z*FRACUNIT;
						//I_Error("Z value: %i", vertexes[l].z/FRACUNIT);

					}
					else
					{
						vertexes[l].z = mt->z*FRACUNIT; // celing floor
					}
					vt_found = true;
				}
			}
			//mt->type = 0; // VPHYSICS: Dynamic slopes






			if (vt_found)
			{
				for (k = 0; k < numsectors; k++)
				{
					sector_t *sec = &sectors[k];
					if (sec->linecount != 3) continue;	// only works with triangular sectors

					v3float_t vt1, vt2, vt3; // cross = ret->normalf
					v3float_t vec1, vec2;

					int vi1, vi2, vi3;

					vi1 = (int)(sec->lines[0]->v1 - vertexes);
					vi2 = (int)(sec->lines[0]->v2 - vertexes);
					vi3 = (sec->lines[1]->v1 == sec->lines[0]->v1 || sec->lines[1]->v1 == sec->lines[0]->v2)?
					(int)(sec->lines[1]->v2 - vertexes) : (int)(sec->lines[1]->v1 - vertexes);

					//if (vertexes[vi1].z)
					//	I_Error("OSNAP %i", vertexes[vi1].z/FRACUNIT);
					//if (vertexes[vi2].z)
					//	I_Error("OSNAP %i", vertexes[vi2].z/FRACUNIT);
					//if (vertexes[vi3].z)
					//	I_Error("OSNAP %i", vertexes[vi3].z/FRACUNIT);

					//I_Error("%i, %i", mt->z*FRACUNIT, vertexes[vi1].z);

					//I_Error("%i, %i, %i", mt->x, mt->y, mt->z);
					//P_SpawnMobj(mt->x*FRACUNIT, mt->y*FRACUNIT, mt->z*FRACUNIT, MT_RING);

					// TODO: Make sure not to spawn in the same place 2x! (we need an object in every vertex of the
					// triangle sector to setup the real vertex slopes
					// Check for the vertexes of all sectors
					for(q = 0; q < numvertexes; q++)
					{
						if (vertexes[q].x == mt->x*FRACUNIT && vertexes[q].y == mt->y*FRACUNIT)
						{
							//I_Error("yeah %i", vertexes[q].z);
							P_SpawnMobj(vertexes[q].x, vertexes[q].y, vertexes[q].z, MT_RING);
#if 0
					if ((mt->y*FRACUNIT == vertexes[vi1].y && mt->x*FRACUNIT == vertexes[vi1].x && mt->z*FRACUNIT == vertexes[vi1].z)
						&& !(mt->y*FRACUNIT == vertexes[vi2].y && mt->x*FRACUNIT == vertexes[vi2].x && mt->z*FRACUNIT == vertexes[vi2].z)
						&& !(mt->y*FRACUNIT == vertexes[vi3].y && mt->x*FRACUNIT == vertexes[vi3].x && mt->z*FRACUNIT == vertexes[vi3].z))
						P_SpawnMobj(vertexes[vi1].x, vertexes[vi1].y, vertexes[vi1].z, MT_RING);
					else if ((mt->y*FRACUNIT == vertexes[vi2].y && mt->x*FRACUNIT == vertexes[vi2].x && mt->z*FRACUNIT == vertexes[vi2].z)
						&& !(mt->y*FRACUNIT == vertexes[vi1].y && mt->x*FRACUNIT == vertexes[vi1].x && mt->z*FRACUNIT == vertexes[vi1].z)
						&& !(mt->y*FRACUNIT == vertexes[vi3].y && mt->x*FRACUNIT == vertexes[vi3].x && mt->z*FRACUNIT == vertexes[vi3].z))
						P_SpawnMobj(vertexes[vi2].x, vertexes[vi2].y, vertexes[vi2].z, MT_BOUNCETV);
					else if ((mt->y*FRACUNIT == vertexes[vi3].y && mt->x*FRACUNIT == vertexes[vi3].x && mt->z*FRACUNIT == vertexes[vi3].z)
						&& !(mt->y*FRACUNIT == vertexes[vi2].y && mt->x*FRACUNIT == vertexes[vi2].x && mt->z*FRACUNIT == vertexes[vi2].z)
						&& !(mt->y*FRACUNIT == vertexes[vi1].y && mt->x*FRACUNIT == vertexes[vi1].x && mt->z*FRACUNIT == vertexes[vi1].z))
						P_SpawnMobj(vertexes[vi3].x, vertexes[vi3].y, vertexes[vi3].z, MT_GFZFLOWER1);
					else
#endif
						continue;
						}
					}

					vt1.x = FIXED_TO_FLOAT(vertexes[vi1].x);
					vt1.y = FIXED_TO_FLOAT(vertexes[vi1].y);
					vt2.x = FIXED_TO_FLOAT(vertexes[vi2].x);
					vt2.y = FIXED_TO_FLOAT(vertexes[vi2].y);
					vt3.x = FIXED_TO_FLOAT(vertexes[vi3].x);
					vt3.y = FIXED_TO_FLOAT(vertexes[vi3].y);

					for(j = 0; j < 2; j++)
					{

						fixed_t z3;
						//I_Error("Lo hicimos");

						vt1.z = mt->z;//FIXED_TO_FLOAT(j==0 ? sec->floorheight : sec->ceilingheight);
						vt2.z = mt->z;//FIXED_TO_FLOAT(j==0? sec->floorheight : sec->ceilingheight);
						z3 = mt->z;//j==0? sec->floorheight : sec->ceilingheight; // Destination height
						vt3.z = FIXED_TO_FLOAT(z3);

						if (P_PointOnLineSide(vertexes[vi3].x, vertexes[vi3].y, sec->lines[0]) == 0)
						{
							vec1.x = vt2.x - vt3.x;
							vec1.y = vt2.y - vt3.y;
							vec1.z = vt2.z - vt3.z;

							vec2.x = vt1.x - vt3.x;
							vec2.y = vt1.y - vt3.y;
							vec2.z = vt1.z - vt3.z;
						}
						else
						{
							vec1.x = vt1.x - vt3.x;
							vec1.y = vt1.y - vt3.y;
							vec1.z = vt1.z - vt3.z;

							vec2.x = vt2.x - vt3.x;
							vec2.y = vt2.y - vt3.y;
							vec2.z = vt2.z - vt3.z;
						}


						pslope_t *ret = Z_Malloc(sizeof(pslope_t), PU_LEVEL, NULL);
						memset(ret, 0, sizeof(*ret));

						{
							M_CrossProduct3f(&ret->normalf, &vec1, &vec2);

							// Cross product length
							float len = (float)sqrt(ret->normalf.x * ret->normalf.x +
													ret->normalf.y * ret->normalf.y +
													ret->normalf.z * ret->normalf.z);

							if (len == 0)
							{
								// Only happens when all vertices in this sector are on the same line.
								// Let's just ignore this case.
								//CONS_Printf("Slope thing at (%d,%d) lies directly on its target line.\n", (int)(x>>16), (int)(y>>16));
								return;
							}
							// cross/len
							ret->normalf.x /= len;
							ret->normalf.y /= len;
							ret->normalf.z /= len;

							// ZDoom cross = ret->normalf
							// Fix backward normals
							if ((ret->normalf.z < 0 && j == 0) || (ret->normalf.z > 0 && j == 1))
							{
								// cross = -cross
								ret->normalf.x = -ret->normalf.x;
								ret->normalf.y = -ret->normalf.x;
								ret->normalf.z = -ret->normalf.x;
							}
						}

						secplane_t *srcplane = Z_Calloc(sizeof(*srcplane), PU_LEVEL, NULL);

						srcplane->a = FLOAT_TO_FIXED (ret->normalf.x);
						srcplane->b = FLOAT_TO_FIXED (ret->normalf.y);
						srcplane->c = FLOAT_TO_FIXED (ret->normalf.z);
						//srcplane->ic = FixedDiv(FRACUNIT, srcplane->c);
						srcplane->d = -TMulScale16 (srcplane->a, vertexes[vi3].x,
													srcplane->b, vertexes[vi3].y,
													srcplane->c, z3);

						if (j == 0)
						{
							sec->f_slope = ret;
							sec->f_slope->secplane = *srcplane;
						}
						else if (j == 1)
						{
							sec->c_slope = ret;
							sec->c_slope->secplane = *srcplane;
						}
					}
				}
			}








		}
	}
	Z_Free(datastart);




}
#endif

// Reset the dynamic slopes pointer, and read all of the fancy schmancy slopes
void P_ResetDynamicSlopes(void) {
	size_t i;
#if 1 // Rewrite old specials to new ones, and give a console warning
	boolean warned = false;
#endif

	slopelist = NULL;
	slopecount = 0;

	// We'll handle copy slopes later, after all the tag lists have been made.
	// Yes, this means copied slopes won't affect things' spawning heights. Too bad for you.
	for (i = 0; i < numlines; i++)
	{
		switch (lines[i].special)
		{
#if 1 // Rewrite old specials to new ones, and give a console warning
#define WARNME if (!warned) {warned = true; CONS_Alert(CONS_WARNING, "This level uses old slope specials.\nA conversion will be needed before 2.2's release.\n");}
			case 386:
			case 387:
			case 388:
				lines[i].special += 700-386;
				WARNME
				P_SpawnSlope_Line(i);
				break;

			case 389:
			case 390:
			case 391:
			case 392:
				lines[i].special += 710-389;
				WARNME
				P_SpawnSlope_Line(i);
				break;

			case 393:
				lines[i].special = 703;
				WARNME
				P_SpawnSlope_Line(i);
				break;

			case 394:
			case 395:
			case 396:
				lines[i].special += 720-394;
				WARNME
				break;

#endif

			case 700:
			case 701:
			case 702:
			case 703:
			case 710:
			case 711:
			case 712:
			case 713:
				P_SpawnSlope_Line(i);
				break;

			case 704:
			case 705:
			case 714:
			case 715:
				{
					pslope_t **slopetoset;
					size_t which = lines[i].special;

					UINT8 flags = SL_VERTEXSLOPE;
					if (lines[i].flags & ML_NOSONIC)
						flags |= SL_NOPHYSICS;
					if (!(lines[i].flags & ML_NOTAILS))
						flags |= SL_NODYNAMIC;

					if (which == 704)
					{
						slopetoset = &lines[i].frontsector->f_slope;
						which = 0;
					}
					else if (which == 705)
					{
						slopetoset = &lines[i].frontsector->c_slope;
						which = 0;
					}
					else if (which == 714)
					{
						slopetoset = &lines[i].backsector->f_slope;
						which = 1;
					}
					else // 715
					{
						slopetoset = &lines[i].backsector->c_slope;
						which = 1;
					}

					if (lines[i].flags & ML_NOKNUX)
						*slopetoset = P_NewVertexSlope(lines[i].tag, sides[lines[i].sidenum[which]].textureoffset >> FRACBITS,
																			sides[lines[i].sidenum[which]].rowoffset >> FRACBITS, flags);
					else
						*slopetoset = P_NewVertexSlope(lines[i].tag, lines[i].tag, lines[i].tag, flags);

					sides[lines[i].sidenum[which]].sector->hasslope = true;
				}
				break;

			default:
				break;
		}
	}
}




// ============================================================================
//
// Various utilities related to slopes
//

//
// P_GetZAt
//
// Returns the height of the sloped plane at (x, y) as a fixed_t
//
fixed_t P_GetZAt(pslope_t *slope, fixed_t x, fixed_t y)
{
   fixed_t dist = FixedMul(x - slope->o.x, slope->d.x) +
                  FixedMul(y - slope->o.y, slope->d.y);

   return slope->o.z + FixedMul(dist, slope->zdelta);
}


//
// P_QuantizeMomentumToSlope
//
// When given a vector, rotates it and aligns it to a slope
void P_QuantizeMomentumToSlope(vector3_t *momentum, pslope_t *slope)
{
	vector3_t axis;
	axis.x = -slope->d.y;
	axis.y = slope->d.x;
	axis.z = 0;

	FV3_Rotate(momentum, &axis, slope->zangle >> ANGLETOFINESHIFT);
}

//
// P_SlopeLaunch
//
// Handles slope ejection for objects
void P_SlopeLaunch(mobj_t *mo)
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

	//CONS_Printf("Launched off of slope.\n");
	mo->standingslope = NULL;
}

// Function to help handle landing on slopes
void P_HandleSlopeLanding(mobj_t *thing, pslope_t *slope)
{
	vector3_t mom;
	mom.x = thing->momx;
	mom.y = thing->momy;
	mom.z = thing->momz*2;

	//CONS_Printf("langing on slope\n");

	// Reverse quantizing might could use its own function later
	slope->zangle = ANGLE_MAX-slope->zangle;
	P_QuantizeMomentumToSlope(&mom, slope);
	slope->zangle = ANGLE_MAX-slope->zangle;

	if (P_MobjFlip(thing)*mom.z < 0) { // falling, land on slope
		thing->momx = mom.x;
		thing->momy = mom.y;
		thing->momz = -P_MobjFlip(thing);

		thing->standingslope = slope;
	}
}

// https://yourlogicalfallacyis.com/slippery-slope
// Handles sliding down slopes, like if they were made of butter :)
void P_ButteredSlope(mobj_t *mo)
{
	fixed_t thrust;

	if (!mo->standingslope)
		return;

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

		//CONS_Printf("%d\n", mult);

		thrust = FixedMul(thrust, FRACUNIT*2/3 + mult/8);
	}

	if (mo->momx || mo->momy) // Slightly increase thrust based on the object's speed
		thrust = FixedMul(thrust, FRACUNIT+P_AproxDistance(mo->momx, mo->momy)/16);
	// This makes it harder to zigzag up steep slopes, as well as allows greater top speed when rolling down

	// Multiply by gravity
	thrust = FixedMul(thrust, FRACUNIT/2); // TODO actually get this

	P_Thrust(mo, mo->standingslope->xydirection, thrust);
}

// EOF
#endif // #ifdef ESLOPE
