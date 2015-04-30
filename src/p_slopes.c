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
#include "p_spec.h"
#include "p_slopes.h"
#include "r_main.h"
#include "p_maputl.h"
#include "w_wad.h"

#ifdef ESLOPE

static pslope_t *dynslopes = NULL;

// Reset the dynamic slopes pointer
void P_ResetDynamicSlopes(void) {
	dynslopes = NULL;
}

// Calculate line normal
void P_CalculateSlopeNormal(pslope_t *slope) {
	slope->normal.z = FINECOSINE(slope->zangle>>ANGLETOFINESHIFT);
	slope->normal.x = -FixedMul(FINESINE(slope->zangle>>ANGLETOFINESHIFT), slope->d.x);
	slope->normal.y = -FixedMul(FINESINE(slope->zangle>>ANGLETOFINESHIFT), slope->d.y);
}

// Recalculate dynamic slopes
void P_RunDynamicSlopes(void) {
	pslope_t *slope;

	for (slope = dynslopes; slope; slope = slope->next) {
		fixed_t zdelta;

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

		default:
			I_Error("P_RunDynamicSlopes: slope has invalid type!");
		}

		if (slope->zdelta != FixedDiv(zdelta, slope->extent)) {
			slope->zdeltaf = FIXED_TO_FLOAT(slope->zdelta = FixedDiv(zdelta, slope->extent));
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
static pslope_t *P_MakeSlope(const v3fixed_t *o, const v2fixed_t *d,
                             const fixed_t zdelta, boolean dynamic)
{
	pslope_t *ret = Z_Malloc(sizeof(pslope_t), PU_LEVEL, NULL);
	memset(ret, 0, sizeof(*ret));

	ret->of.x = FIXED_TO_FLOAT(ret->o.x = o->x);
	ret->of.y = FIXED_TO_FLOAT(ret->o.y = o->y);
	ret->of.z = FIXED_TO_FLOAT(ret->o.z = o->z);

	ret->df.x = FIXED_TO_FLOAT(ret->d.x = d->x);
	ret->df.y = FIXED_TO_FLOAT(ret->d.y = d->y);

	ret->zdeltaf = FIXED_TO_FLOAT(ret->zdelta = zdelta);

	if (dynamic) { // Add to the dynamic slopes list
		ret->next = dynslopes;
		dynslopes = ret;
	}

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
	v3fixed_t origin, point;
	v2fixed_t direction;
	fixed_t nx, ny, dz, extent;

	boolean frontfloor = (special == 386 || special == 388 || special == 393);
	boolean backfloor  = (special == 389 || special == 391 || special == 392);
	boolean frontceil  = (special == 387 || special == 388 || special == 392);
	boolean backceil   = (special == 390 || special == 391 || special == 393);

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

			point.z = line->frontsector->floorheight; // Startz
			dz = FixedDiv(origin.z - point.z, extent); // Destinationz

			// In P_SpawnSlopeLine the origin is the centerpoint of the sourcelinedef

			fslope = line->frontsector->f_slope =
            P_MakeSlope(&point, &direction, dz, true);

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
			fixed_t highest = point.z > origin.z ? point.z : origin.z;
			fixed_t lowest = point.z < origin.z ? point.z : origin.z;

			// Now check to see what the REAL high and low points of the slope inside the sector
			// TODO: Is this really needed outside of FOFs? -Red
			size_t l;

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
			origin.z = line->backsector->ceilingheight;
			point.z = line->frontsector->ceilingheight;
			dz = FixedDiv(origin.z - point.z, extent);

			cslope = line->frontsector->c_slope =
            P_MakeSlope(&point, &direction, dz, true);

            // Set up some shit
            cslope->extent = extent;
            cslope->refpos = 2;

			// Sync the linedata of the line that started this slope
			// SRB2CBTODO: Anything special for remote(control sector)-based slopes later?
			cslope->sourceline = line;

			// Remember the way the slope is formed
			fixed_t highest = point.z > origin.z ? point.z : origin.z;
			fixed_t lowest = point.z < origin.z ? point.z : origin.z;
			size_t l;

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
			point.z = line->backsector->floorheight;
			dz = FixedDiv(origin.z - point.z, extent);

			fslope = line->backsector->f_slope =
            P_MakeSlope(&point, &direction, dz, true);

            // Set up some shit
            fslope->extent = extent;
            fslope->refpos = 3;

			// Sync the linedata of the line that started this slope
			// SRB2CBTODO: Anything special for remote(control sector)-based slopes later?
			fslope->sourceline = line;

			// Remember the way the slope is formed
			fixed_t highest = point.z > origin.z ? point.z : origin.z;
			fixed_t lowest = point.z < origin.z ? point.z : origin.z;
			size_t l;

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
			origin.z = line->frontsector->ceilingheight;
			point.z = line->backsector->ceilingheight;
			dz = FixedDiv(origin.z - point.z, extent);

			cslope = line->backsector->c_slope =
            P_MakeSlope(&point, &direction, dz, true);

            // Set up some shit
            cslope->extent = extent;
            cslope->refpos = 4;

			// Sync the linedata of the line that started this slope
			// SRB2CBTODO: Anything special for remote(control sector)-based slopes later?
			cslope->sourceline = line;

			// Remember the way the slope is formed
			fixed_t highest = point.z > origin.z ? point.z : origin.z;
			fixed_t lowest = point.z < origin.z ? point.z : origin.z;

			size_t l;

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

      if((special - 393) & 1 && !fsec->f_slope && srcsec->f_slope)
         fsec->f_slope = srcsec->f_slope; //P_CopySlope(srcsec->f_slope);
      if((special - 393) & 2 && !fsec->c_slope && srcsec->c_slope)
         fsec->c_slope = srcsec->c_slope; //P_CopySlope(srcsec->c_slope);
   }

   line->special = 0; // Linedef was use to set slopes, it finished its job, so now make it a normal linedef
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
// P_GetZAtf
//
// Returns the height of the sloped plane at (x, y) as a float
//
float P_GetZAtf(pslope_t *slope, float x, float y)
{
	//if (!slope) // SRB2CBTODO: keep this when done with debugging
	//	I_Error("P_GetZAtf: slope parameter is NULL");

   float dist = (x - slope->of.x) * slope->df.x + (y - slope->of.y) * slope->df.y;
   return slope->of.z + (dist * slope->zdeltaf);
}

// Unused? -Red
// P_DistFromPlanef
//
float P_DistFromPlanef(const v3float_t *point, const v3float_t *pori,
                       const v3float_t *pnormal)
{
   return (point->x - pori->x) * pnormal->x +
          (point->y - pori->y) * pnormal->y +
          (point->z - pori->z) * pnormal->z;
}

//
// P_QuantizeMomentumToSlope
//
// When given a vector, rotates it and aligns it to a slope
void P_QuantizeMomentumToSlope(v3fixed_t *momentum, pslope_t *slope)
{
	v3fixed_t axis;
	axis.x = -slope->d.y;
	axis.y = slope->d.x;
	axis.z = 0;

	M_VecRotate(momentum, &axis, slope->zangle);
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
	v3fixed_t slopemom;
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

// https://yourlogicalfallacyis.com/slippery-slope
// Handles sliding down slopes, like if they were made of butter :)
void P_ButteredSlope(mobj_t *mo)
{
	fixed_t thrust;

	if (!mo->standingslope)
		return;

	if (abs(mo->standingslope->zdelta) < FRACUNIT/3)
		return; // Don't apply physics to slopes that aren't steep enough

	thrust = FINESINE(mo->standingslope->zangle>>ANGLETOFINESHIFT) * 3 / 2 * (mo->eflags & MFE_VERTICALFLIP ? 1 : -1);

	if (mo->momx || mo->momy) // Slightly increase thrust based on the object's speed
		thrust = FixedMul(thrust, FRACUNIT+P_AproxDistance(mo->momx, mo->momy)/16);
	// This makes it harder to zigzag up steep slopes, as well as allows greater top speed when rolling down

	// Multiply by gravity
	thrust = FixedMul(thrust, FRACUNIT/2); // TODO actually get this

	P_Thrust(mo, mo->standingslope->xydirection, thrust);
}

// EOF
#endif // #ifdef ESLOPE

