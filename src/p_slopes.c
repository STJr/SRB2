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

//
// P_MakeSlope
//
// Alocates and fill the contents of a slope structure.
//
static pslope_t *P_MakeSlope(const v3float_t *o, const v2float_t *d, 
                             const float zdelta, boolean isceiling)
{
   pslope_t *ret = Z_Malloc(sizeof(pslope_t), PU_LEVEL, NULL);
   memset(ret, 0, sizeof(*ret));

   ret->o.x = FLOAT_TO_FIXED(ret->of.x = o->x);
   ret->o.y = FLOAT_TO_FIXED(ret->of.y = o->y);
   ret->o.z = FLOAT_TO_FIXED(ret->of.z = o->z);

   ret->d.x = FLOAT_TO_FIXED(ret->df.x = d->x);
   ret->d.y = FLOAT_TO_FIXED(ret->df.y = d->y);

   ret->zdelta = FLOAT_TO_FIXED(ret->zdeltaf = zdelta);
	
	// d = direction (v2float_t)
	//
	//   direction.x = line->nx;
	//   direction.y = line->ny;
	//
	// o = origin (v3float_t)
	//   origin.x = (FIXED_TO_FLOAT(line->v2->x) + FIXED_TO_FLOAT(line->v1->x)) * 0.5f;
	//   origin.y = (FIXED_TO_FLOAT(line->v2->y) + FIXED_TO_FLOAT(line->v1->y)) * 0.5f;

   {
	   // Now calculate the normal of the plane!
      v3float_t v1, v2, v3, d1, d2;
      float len;

      v1.x = o->x;
      v1.y = o->y;
      v1.z = o->z;

      v2.x = v1.x;
      v2.y = v1.y + 10.0f;
      v2.z = P_GetZAtf(ret, v2.x, v2.y);

      v3.x = v1.x + 10.0f;
      v3.y = v1.y;
      v3.z = P_GetZAtf(ret, v3.x, v3.y);

      if (isceiling)
      {
         M_SubVec3f(&d1, &v1, &v3);
         M_SubVec3f(&d2, &v2, &v3);
      }
      else
      {
         M_SubVec3f(&d1, &v1, &v2);
         M_SubVec3f(&d2, &v3, &v2);
      }

      M_CrossProduct3f(&ret->normalf, &d1, &d2);

	   // Cross product length
      len = (float)sqrt(ret->normalf.x * ret->normalf.x +
                        ret->normalf.y * ret->normalf.y + 
                        ret->normalf.z * ret->normalf.z);
	   
#ifdef SLOPETHINGS
	   if (len == 0)
	   {
		   // Only happens when all vertices in this sector are on the same line.
		   // Let's just ignore this case.
		   CONS_Printf("Slope thing at (%d,%d) lies directly on its target line.\n", int(x>>16), int(y>>16));
		   return;
	   }
#endif

      ret->normalf.x /= len;
      ret->normalf.y /= len;
      ret->normalf.z /= len;
	   
	   // ZDoom
	   // cross = ret->normalf
	   
	   // Fix backward normals
	   if ((ret->normalf.z < 0 && !isceiling) || (ret->normalf.z > 0 && isceiling))
	   {
		   ret->normalf.x = -ret->normalf.x;
		   ret->normalf.y = -ret->normalf.x;
		   ret->normalf.z = -ret->normalf.x;
	   }  
	   
   }

   return ret;
}

//
// P_CopySlope
//
// Allocates and returns a copy of the given slope structure.
//
static pslope_t *P_CopySlope(const pslope_t *src)
{
   pslope_t *ret = Z_Malloc(sizeof(pslope_t), PU_LEVEL, NULL);
   memcpy(ret, src, sizeof(*ret));

   return ret;
}

//
// P_MakeLineNormal
//
// Calculates a 2D normal for the given line and stores it in the line
//
void P_MakeLineNormal(line_t *line)
{
   float linedx, linedy, length;

	// SRB2CBTODO: Give linedefs an fx+fy(float xy coords)?
	// May cause slow downs since the float would always have to be converted/updated
   linedx = FIXED_TO_FLOAT(line->v2->x) - FIXED_TO_FLOAT(line->v1->x);
   linedy = FIXED_TO_FLOAT(line->v2->y) - FIXED_TO_FLOAT(line->v1->y);

   length   = (float)sqrt(linedx * linedx + linedy * linedy);
   line->nx =  linedy / length;
   line->ny = -linedx / length;
   line->len = length;
}

//
// P_GetExtent
//
// Returns the distance to the first line within the sector that
// is intersected by a line parallel to the plane normal with the point (ox, oy)
//
static float P_GetExtent(sector_t *sector, line_t *line, v3float_t *o, v2float_t *d)
{
	// ZDoom code reference: v3float_t = vertex_t
   float fardist = -1.0f;
   size_t i;

	// Find furthest vertex from the reference line. It, along with the two ends
	// of the line, will define the plane.
	// SRB2CBTODO: Use a formula to get the slope to slide objects depending on how steep
   for(i = 0; i < sector->linecount; i++)
   {
      line_t *li = sector->lines[i];
      float dist;
      
      // Don't compare to the slope line.
      if(li == line)
         continue;
      
	   // ZDoom code in P_AlignPlane
	   // dist = fabs((double(line->v1->y) - vert->y) * line->dx - (double(line->v1->x) - vert->x) * line->dy);
      dist = (float)fabs((FIXED_TO_FLOAT(li->v1->x) - o->x) * d->x + (FIXED_TO_FLOAT(li->v1->y) - o->y) * d->y);
      if(dist > fardist)
         fardist = dist;

      dist = (float)fabs((FIXED_TO_FLOAT(li->v2->x) - o->x) * d->x + (FIXED_TO_FLOAT(li->v2->y) - o->y) * d->y);
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
	int special = line->special;
	pslope_t *fslope = NULL, *cslope = NULL;
	v3float_t origin, point;
	v2float_t direction;
	float dz, extent;
	
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
	
	// SRB2CBTODO: Transform origin relative to the bounds of an individual FOF
	origin.x = (FIXED_TO_FLOAT(line->v2->x) + FIXED_TO_FLOAT(line->v1->x)) * 0.5f;
	origin.y = (FIXED_TO_FLOAT(line->v2->y) + FIXED_TO_FLOAT(line->v1->y)) * 0.5f;
	
	// For FOF slopes, make a special function to copy to the xy origin & direction relative to the position of the FOF on the map!
	if(frontfloor || frontceil)
	{
		origin.z = FIXED_TO_FLOAT(line->backsector->floorheight);
		direction.x = line->nx;
		direction.y = line->ny;
		
		extent = P_GetExtent(line->frontsector, line, &origin, &direction);
		
		if(extent < 0.0f)
		{
			CONS_Printf("P_SpawnSlope_Line failed to get frontsector extent on line number %i\n", linenum);
			return;
		}
		
		// reposition the origin according to the extent
		point.x = origin.x + direction.x * extent;
		point.y = origin.y + direction.y * extent;
		direction.x = -direction.x;
		direction.y = -direction.y;
		
		// TODO: We take origin and point 's xy values and translate them to the center of an FOF!
		
		if(frontfloor)
		{
			
			point.z = FIXED_TO_FLOAT(line->frontsector->floorheight); // Startz
			dz = (FIXED_TO_FLOAT(line->backsector->floorheight) - point.z) / extent; // Destinationz
			
			// In P_SpawnSlopeLine the origin is the centerpoint of the sourcelinedef
			
			int slopeangle = 0; // All floors by default have no slope (an angle of 0, completely flat)
			
			v3float_t A = origin; // = line source
			v3float_t B = point; // destination's value
			v3float_t C = origin; // Point used to make a right triangle from A & B
			
			C.z = point.z;
			
			// To find the "angle" of a slope, we make a right triangle out of the points we have,
			// point A - is point 1 of the hypotenuse,
			// point B - is point 2 of the hypotenuse
			// point C - has the same Z value as point b, and the same XY value as A
			// 
			// We want to find the angle accross from the right angle
			// so we use some triginometry to find the angle(fun, right?)
			// We want to find the tanjent of this angle, this is:
			//  Opposite
			//  -------   =  tan(x)
			//  Adjecent
			// But actually tan doesn't do want we really want, we have to use atan to find the actual angle of the triangle's corner
			float triangopplength = abs(B.z - A.z);
			float triangadjlength = sqrt((B.x-C.x)*(B.x-C.x) + (B.y - C.y)*(B.y - C.y));
			//float trianghyplength = sqrt(triangopplength*triangopplength + triangadjlength*triangadjlength); // This is the hypotenuse
			
			// So tanjent = opposite divided by adjecent
			float tanrelat = triangopplength/ triangadjlength; // tanjent = opposite / adjecent
			slopeangle = atan(tanrelat)* 180 / M_PI; // Now we use atan: *180 /M_PI is needed to convert the value into degrees
			
			fslope = line->frontsector->f_slope = 
            P_MakeSlope(&point, &direction, dz, false);
			
			// Now remember that f_slope IS a vector
			// fslope->o = origin      3D point 1 of the vector
			// fslope->d = destination 3D point 2 of the vector
			// fslope->normal is a 3D line perpendicular to the 3D vector
			
			// Sync the linedata of the line that started this slope
			// SRB2CBTODO: Anything special for remote(control sector)-based slopes later?
			line->frontsector->f_slope->sourceline = line;
			
			// To find the real highz/lowz of a slope, you need to check all the vertexes
			// in the slope's sector with P_GetZAt to get the REAL lowz & highz
			// Although these slopes are set by floorheights the ANGLE is what a slope is,
			// so technically any slope can extend on forever (they are just bound by sectors)
			// *You can use sourceline as a reference to see if two slopes really are the same
			
			// Default points for high and low
			fixed_t highest = point.z > origin.z ? point.z : origin.z;
			fixed_t lowest = point.z < origin.z ? point.z : origin.z;
			highest = FLOAT_TO_FIXED(highest);
			lowest = FLOAT_TO_FIXED(lowest);
			
			// Now check to see what the REAL high and low points of the slope inside the sector
			size_t l;
			
			for (l = 0; l < line->frontsector->linecount; l++)
			{
				if (P_GetZAt(line->frontsector->f_slope, line->frontsector->lines[l]->v1->x, line->frontsector->lines[l]->v1->y) > highest)
					highest = P_GetZAt(line->frontsector->f_slope, line->frontsector->lines[l]->v1->x, line->frontsector->lines[l]->v1->y);
				
				if (P_GetZAt(line->frontsector->f_slope, line->frontsector->lines[l]->v1->x, line->frontsector->lines[l]->v1->y) < lowest)
					lowest = P_GetZAt(line->frontsector->f_slope, line->frontsector->lines[l]->v1->x, line->frontsector->lines[l]->v1->y);
			}
			
			// Sets extra clipping data for the frontsector's slope
			fslope->highz = line->frontsector->f_slope->highz = highest;
			fslope->lowz = line->frontsector->f_slope->lowz = lowest;
			
			fslope->zangle = slopeangle;
			fslope->xydirection = R_PointToAngle2(FLOAT_TO_FIXED(A.x), FLOAT_TO_FIXED(A.y), FLOAT_TO_FIXED(B.x), FLOAT_TO_FIXED(B.y))/(ANGLE_45/45);
			
			secplane_t *srcplane = Z_Calloc(sizeof(*srcplane), PU_LEVEL, NULL);
			// ZDoom secplane port! YAY
			// ret = f_slope or c_slope
			srcplane->a = FLOAT_TO_FIXED (fslope->normalf.x); // cross[0]
			srcplane->b = FLOAT_TO_FIXED (fslope->normalf.y); // cross[1]
			srcplane->c = FLOAT_TO_FIXED (fslope->normalf.z); // cross[2]
			srcplane->ic = DivScale32 (1, srcplane->c); // (1 << 32/srcplane->c) or FLOAT_TO_FIXED(1.0f/cross[2]);
			
			// destheight takes the destination height used in dz 
			srcplane->d = -TMulScale16 (srcplane->a, line->v1->x, // x
										srcplane->b, line->v1->y, // y
										srcplane->c, line->backsector->floorheight); // z
			
			// Sync the secplane!
			fslope->secplane = line->frontsector->f_slope->secplane = *srcplane;
			
		}
		if(frontceil)
		{
			point.z = FIXED_TO_FLOAT(line->frontsector->ceilingheight);
			dz = (FIXED_TO_FLOAT(line->backsector->ceilingheight) - point.z) / extent;
			
			cslope = line->frontsector->c_slope = 
            P_MakeSlope(&point, &direction, dz, true);
			
			// Sync the linedata of the line that started this slope
			// SRB2CBTODO: Anything special for remote(control sector)-based slopes later?
			line->frontsector->c_slope->sourceline = line;
			
			// Remember the way the slope is formed
			fixed_t highest = point.z > origin.z ? point.z : origin.z;
			fixed_t lowest = point.z < origin.z ? point.z : origin.z;
			highest = FLOAT_TO_FIXED(highest);
			lowest = FLOAT_TO_FIXED(lowest);
			size_t l;
			
			for (l = 0; l < line->frontsector->linecount; l++)
			{
				if (P_GetZAt(line->frontsector->c_slope, line->frontsector->lines[l]->v1->x, line->frontsector->lines[l]->v1->y) > highest)
					highest = P_GetZAt(line->frontsector->c_slope, line->frontsector->lines[l]->v1->x, line->frontsector->lines[l]->v1->y);
				
				if (P_GetZAt(line->frontsector->c_slope, line->frontsector->lines[l]->v1->x, line->frontsector->lines[l]->v1->y) < lowest)
					lowest = P_GetZAt(line->frontsector->c_slope, line->frontsector->lines[l]->v1->x, line->frontsector->lines[l]->v1->y);
			}
			
			// This line special sets extra clipping data for the frontsector's slope
			cslope->highz = line->frontsector->c_slope->highz = highest;
			cslope->lowz = line->frontsector->c_slope->lowz = lowest;
			
			// SRB2CBTODO: Get XY angle of a slope and then awesome physics! //        ESLOPE:
			//cslope->zangle = line->frontsector->c_slope->zangle = P_GetSlopezangle(line->frontsector, highvert, lowvert);
			//100*(ANG45/45);//R_PointToAngle2(direction.x, direction.y, origin.x, origin.y);
			// Get slope XY angle with secplane_t
			secplane_t *srcplane = Z_Calloc(sizeof(*srcplane), PU_LEVEL, NULL);
			// ZDoom secplane port!
			// secplane_t! woot!
			// ret = f_slope or c_slope
			srcplane->a = FLOAT_TO_FIXED (cslope->normalf.x); // cross[0]
			srcplane->b = FLOAT_TO_FIXED (cslope->normalf.y); // cross[1]
			srcplane->c = FLOAT_TO_FIXED (cslope->normalf.z); // cross[2]
			//plane->ic = FLOAT_TO_FIXED (1.f/cross[2]);
			srcplane->ic = DivScale32 (1, srcplane->c); // (1 << 32/srcplane->c)
#ifdef SLOPETHINGS // For setting thing-based slopes
			srcplane->d = -TMulScale16 (plane->a, x,
										plane->b, y,
										plane->c, z);
#endif
			//srcheight = isceiling ? sec->GetPlaneTexZ(sector_t::floor) : sec->GetPlaneTexZ(sector_t::ceiling);
			//destheight = isceiling ? refsec->GetPlaneTexZ(sector_t::floor) : refsec->GetPlaneTexZ(sector_t::ceiling);
			//P_GetZAtf(ret, v2.x, v2.y)
			// destheight takes the destination height used in dz 
			srcplane->d = -TMulScale16 (srcplane->a, line->v1->x,
										srcplane->b, line->v1->y,
										srcplane->c, line->backsector->ceilingheight);
			
			// Sync the secplane!
			cslope->secplane = line->frontsector->c_slope->secplane = *srcplane;
		}
	}
	if(backfloor || backceil)
	{
		origin.z = FIXED_TO_FLOAT(line->frontsector->floorheight);
		// Backsector
		direction.x = -line->nx;
		direction.y = -line->ny;
		
		extent = P_GetExtent(line->backsector, line, &origin, &direction);
		
		if(extent < 0.0f)
		{
			CONS_Printf("P_SpawnSlope_Line failed to get backsector extent on line number %i\n", linenum);
			return;
		}
		
		// reposition the origin according to the extent
		point.x = origin.x + direction.x * extent;
		point.y = origin.y + direction.y * extent;
		direction.x = -direction.x;
		direction.y = -direction.y;
		
		if(backfloor)
		{
			point.z = FIXED_TO_FLOAT(line->backsector->floorheight);
			dz = (FIXED_TO_FLOAT(line->frontsector->floorheight) - point.z) / extent;
			
			fslope = line->backsector->f_slope = 
            P_MakeSlope(&point, &direction, dz, false);
			
			// Sync the linedata of the line that started this slope
			// SRB2CBTODO: Anything special for remote(control sector)-based slopes later?
			line->backsector->f_slope->sourceline = line;
			
			int slopeangle = 0; // All floors by default have no slope (an angle of 0)
			
			v3float_t A = origin; // = line source
			v3float_t B = point; // destination's value
			v3float_t C = origin;
			
			C.z = point.z;
			
			// To find the "angle" of a slope, we make a right triangle out of the points we have,
			// point A - is point 1 of the hypotenuse,
			// point B - is point 2 of the hypotenuse
			// point C - has the same Z value as point b, and the same XY value as A
			// 
			// We want to find the angle accross from the right angle
			// so we use some triginometry to find the angle(fun, right?)
			// We want to find the tanjent of this angle, this is:
			//  Opposite
			//  -------   =  tan(x)
			//  Adjecent
			// But actually tan doesn't do want we really want, we have to use atan to find the actual angle of the triangle's corner
			float triangopplength = abs(B.z - A.z);
			float triangadjlength = sqrt((B.x-C.x)*(B.x-C.x) + (B.y - C.y)*(B.y - C.y));
			//float trianghyplength = sqrt(triangopplength*triangopplength + triangadjlength*triangadjlength); // This is the hypotenuse
			
			// So tanjent = opposite divided by adjecent
			float tanrelat = triangopplength/ triangadjlength; // tanjent = opposite / adjecent
			slopeangle = atan(tanrelat)* 180 / M_PI; // Now we use atan - *180 /M_PI is needed to convert the value into degrees
			
			// Remember the way the slope is formed
			fixed_t highest = point.z > origin.z ? point.z : origin.z;
			fixed_t lowest = point.z < origin.z ? point.z : origin.z;
			highest = FLOAT_TO_FIXED(highest);
			lowest = FLOAT_TO_FIXED(lowest);
			size_t l;
			
			for (l = 0; l < line->backsector->linecount; l++)
			{
				if (P_GetZAt(line->backsector->f_slope, line->backsector->lines[l]->v1->x, line->backsector->lines[l]->v1->y) > highest)
					highest = P_GetZAt(line->backsector->f_slope, line->backsector->lines[l]->v1->x, line->backsector->lines[l]->v1->y);
				
				if (P_GetZAt(line->backsector->f_slope, line->backsector->lines[l]->v1->x, line->backsector->lines[l]->v1->y) < lowest)
					lowest = P_GetZAt(line->backsector->f_slope, line->backsector->lines[l]->v1->x, line->backsector->lines[l]->v1->y);
			}
			
			// This line special sets extra clipping data for the frontsector's slope
			fslope->highz = line->backsector->f_slope->highz = highest;
			fslope->lowz = line->backsector->f_slope->lowz = lowest;
			
			fslope->zangle = slopeangle;
			// Get slope XY angle with secplane_t
			secplane_t *srcplane = Z_Calloc(sizeof(*srcplane), PU_LEVEL, NULL);
			// ZDoom secplane port!
			// secplane_t! woot!
			// ret = f_slope or c_slope
			srcplane->a = FLOAT_TO_FIXED (fslope->normalf.x); // cross[0]
			srcplane->b = FLOAT_TO_FIXED (fslope->normalf.y); // cross[1]
			srcplane->c = FLOAT_TO_FIXED (fslope->normalf.z); // cross[2]
			//plane->ic = FLOAT_TO_FIXED (1.f/cross[2]);
			srcplane->ic = DivScale32 (1, srcplane->c); // (1 << 32/srcplane->c)
#ifdef SLOPETHINGS // For setting thing-based slopes
			srcplane->d = -TMulScale16 (plane->a, x,
										plane->b, y,
										plane->c, z);
#endif
			//srcheight = isceiling ? sec->GetPlaneTexZ(sector_t::floor) : sec->GetPlaneTexZ(sector_t::ceiling);
			//destheight = isceiling ? refsec->GetPlaneTexZ(sector_t::floor) : refsec->GetPlaneTexZ(sector_t::ceiling);
			//P_GetZAtf(ret, v2.x, v2.y)
			// destheight takes the destination height used in dz 
			srcplane->d = -TMulScale16 (srcplane->a, line->v1->x,
										srcplane->b, line->v1->y,
										srcplane->c, line->frontsector->floorheight);
			
			// Sync the secplane!
			fslope->secplane = line->backsector->f_slope->secplane = *srcplane;
		}
		if(backceil)
		{
			point.z = FIXED_TO_FLOAT(line->backsector->ceilingheight);
			dz = (FIXED_TO_FLOAT(line->frontsector->ceilingheight) - point.z) / extent;
			
			cslope = line->backsector->c_slope = 
            P_MakeSlope(&point, &direction, dz, true);
			
			// Sync the linedata of the line that started this slope
			// SRB2CBTODO: Anything special for remote(control sector)-based slopes later?
			line->backsector->c_slope->sourceline = line;
			
			// Remember the way the slope is formed
			fixed_t highest = point.z > origin.z ? point.z : origin.z;
			fixed_t lowest = point.z < origin.z ? point.z : origin.z;
			highest = FLOAT_TO_FIXED(highest);
			lowest = FLOAT_TO_FIXED(lowest);
			
			size_t l;
			
			for (l = 0; l < line->backsector->linecount; l++)
			{
				if (P_GetZAt(line->backsector->c_slope, line->backsector->lines[l]->v1->x, line->backsector->lines[l]->v1->y) > highest)
					highest = P_GetZAt(line->backsector->c_slope, line->backsector->lines[l]->v1->x, line->backsector->lines[l]->v1->y);
				
				if (P_GetZAt(line->backsector->c_slope, line->backsector->lines[l]->v1->x, line->backsector->lines[l]->v1->y) < lowest)
					lowest = P_GetZAt(line->backsector->c_slope, line->backsector->lines[l]->v1->x, line->backsector->lines[l]->v1->y);
			}
			
			// This line special sets extra clipping data for the backsector's slope
			cslope->highz = line->backsector->c_slope->highz = highest;
			cslope->lowz = line->backsector->c_slope->lowz = lowest;
			
			// SRB2CBTODO: Get XY angle of a slope and then awesome physics! //        ESLOPE:
			//cslope->zangle = line->backsector->c_slope->zangle = P_GetSlopezangle(line->backsector, highvert, lowvert);
			//100*(ANG45/45);//R_PointToAngle2(direction.x, direction.y, origin.x, origin.y);
			// Get slope XY angle with secplane_t
			secplane_t *srcplane = Z_Calloc(sizeof(*srcplane), PU_LEVEL, NULL);
			// ZDoom secplane port!
			// secplane_t! woot!
			// ret = f_slope or c_slope
			srcplane->a = FLOAT_TO_FIXED (cslope->normalf.x); // cross[0]
			srcplane->b = FLOAT_TO_FIXED (cslope->normalf.y); // cross[1]
			srcplane->c = FLOAT_TO_FIXED (cslope->normalf.z); // cross[2]
			//plane->ic = FLOAT_TO_FIXED (1.f/cross[2]);
			srcplane->ic = DivScale32 (1, srcplane->c); // (1 << 32/srcplane->c)
#ifdef SLOPETHINGS // For setting thing-based slopes
			srcplane->d = -TMulScale16 (plane->a, x,
										plane->b, y,
										plane->c, z);
#endif
			//srcheight = isceiling ? sec->GetPlaneTexZ(sector_t::floor) : sec->GetPlaneTexZ(sector_t::ceiling);
			//destheight = isceiling ? refsec->GetPlaneTexZ(sector_t::floor) : refsec->GetPlaneTexZ(sector_t::ceiling);
			//P_GetZAtf(ret, v2.x, v2.y)
			// destheight takes the destination height used in dz 
			srcplane->d = -TMulScale16 (srcplane->a, line->v1->x,
										srcplane->b, line->v1->y,
										srcplane->c, line->frontsector->ceilingheight);
			
			// Sync the secplane!
			cslope->secplane = line->backsector->c_slope->secplane = *srcplane;
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
         fsec->f_slope = P_CopySlope(srcsec->f_slope);
      if((special - 393) & 2 && !fsec->c_slope && srcsec->c_slope)
         fsec->c_slope = P_CopySlope(srcsec->c_slope);
   }

	//SRB2CBTODO: ESLOPE: Maybe we do need it for another to check for a plane slope?
   line->special = 0; // Linedef was use to set slopes, it finished its job, so now make it a normal linedef
}


#include "byteptr.h"

/*
typedef struct
{
	fixed_t z1;
	fixed_t z2;
} mapvert_t;*/

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
						//srcplane->ic = DivScale32 (1, srcplane->c);
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
	
#if 0 // UDMF support
	for(i = 0; i < numvertexdatas; i++)
	{
		if (vertexdatas[i].flags & VERTEXFLAG_ZCeilingEnabled)
		{
			vt_heights[1][i] = vertexdatas[i].zCeiling;
			vt_found = true;
		}
		
		if (vertexdatas[i].flags & VERTEXFLAG_ZFloorEnabled)
		{
			vt_heights[0][i] = vertexdatas[i].zFloor;
			vt_found = true;
		}
	}
	
	// If vertexdata_t is ever extended for non-slope usage, this will obviously have to be deferred or removed.
	delete[] vertexdatas;
	vertexdatas = NULL;
	numvertexdatas = 0;
#endif
	

	
	
}

#include "p_maputl.h"

#if 0
static void P_SlopeLineToPointo (int lineid, fixed_t x, fixed_t y, fixed_t z, boolean slopeCeil)
{
	int linenum = -1;
	
	while ((linenum = P_FindLineFromID (lineid, linenum)) != -1)
	{
		const line_t *line = &lines[linenum];
		sector_t *sec;
		secplane_t *plane;
		
		if (P_PointOnLineSide (x, y, line) == 0)
		{
			sec = line->frontsector;
		}
		else
		{
			sec = line->backsector;
		}
		if (sec == NULL)
		{
			continue;
		}
		if (slopeCeil)
		{
			plane = &sec->ceilingplane;
		}
		else
		{
			plane = &sec->floorplane;
		}
		
		FVector3 p, v1, v2, cross;
		
		p[0] = FIXED2FLOAT (line->v1->x);
		p[1] = FIXED2FLOAT (line->v1->y);
		p[2] = FIXED2FLOAT (plane->ZatPoint (line->v1->x, line->v1->y));
		v1[0] = FIXED2FLOAT (line->dx);
		v1[1] = FIXED2FLOAT (line->dy);
		v1[2] = FIXED2FLOAT (plane->ZatPoint (line->v2->x, line->v2->y)) - p[2];
		v2[0] = FIXED2FLOAT (x - line->v1->x);
		v2[1] = FIXED2FLOAT (y - line->v1->y);
		v2[2] = FIXED2FLOAT (z) - p[2];
		
		cross = v1 ^ v2;
		double len = cross.Length();
		if (len == 0)
		{
			Printf ("Slope thing at (%d,%d) lies directly on its target line.\n", int(x>>16), int(y>>16));
			return;
		}
		cross /= len;
		// Fix backward normals
		if ((cross.Z < 0 && !slopeCeil) || (cross.Z > 0 && slopeCeil))
		{
			cross = -cross;
		}
		
		plane->a = FLOAT2FIXED (cross[0]);
		plane->b = FLOAT2FIXED (cross[1]);
		plane->c = FLOAT2FIXED (cross[2]);
		//plane->ic = FLOAT2FIXED (1.f/cross[2]);
		plane->ic = DivScale32 (1, plane->c);
		plane->d = -TMulScale16 (plane->a, x,
								 plane->b, y,
								 plane->c, z);
	}
}
#else
#if 0
// P_SlopeLineToPoint, start from a specific linedef number(not tag) and slope to a mapthing with the angle of the linedef 
static void P_SlopeLineToPoint(int linenum)
{
	line_t *line = lines + linenum;
	int special = line->special;
	pslope_t *fslope = NULL, *cslope = NULL;
	v3float_t origin, point;
	v2float_t direction;
	float dz, extent;
	
	boolean frontfloor = (special == 386 || special == 388 || special == 393);
	boolean backfloor  = (special == 389 || special == 391 || special == 392);
	boolean frontceil  = (special == 387 || special == 388 || special == 392);
	boolean backceil   = (special == 390 || special == 391 || special == 393);
	
	// SoM: We don't need the line to retain its special type
	line->special = 0; //SRB2CBTODO: ESLOPE: Maybe we do need it for another to check for a plane slope?
	
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
	
	origin.x = (FIXED_TO_FLOAT(line->v2->x) + FIXED_TO_FLOAT(line->v1->x)) * 0.5f;
	origin.y = (FIXED_TO_FLOAT(line->v2->y) + FIXED_TO_FLOAT(line->v1->y)) * 0.5f;
	
	if(frontfloor || frontceil)
	{
		// Do the front sector
		direction.x = line->nx;
		direction.y = line->ny;
		
		extent = P_GetExtent(line->frontsector, line, &origin, &direction);
		
		if(extent < 0.0f)
		{
			CONS_Printf("P_SpawnSlope_Line failed to get frontsector extent on line number %i\n", linenum);
			return;
		}
		
		// reposition the origin according to the extent
		point.x = origin.x + direction.x * extent;
		point.y = origin.y + direction.y * extent;
		direction.x = -direction.x;
		direction.y = -direction.y;
		
		// CONS_Printf("Test: X: %f, Y: %f\n", origin.x, origin.y);
		
		if(frontfloor)
		{
			point.z = FIXED_TO_FLOAT(line->frontsector->floorheight); // Startz
			dz = (FIXED_TO_FLOAT(line->backsector->floorheight) - point.z) / extent; // Destinationz
			
			fslope = line->frontsector->f_slope = 
            P_MakeSlope(&point, &direction, dz, false);
			
			// Sync the linedata of the line that started this slope
			// SRB2CBTODO: Anything special for remote(control sector)-based slopes later?
			line->frontsector->f_slope->sourceline = line;
			
			// Remember the way the slope is formed
			fixed_t highest = line->frontsector->floorheight > line->backsector->floorheight ? 
			line->frontsector->floorheight : line->backsector->floorheight;
			fixed_t lowest = line->frontsector->floorheight < line->backsector->floorheight ? 
			line->frontsector->floorheight : line->backsector->floorheight;
			// This line special sets extra clipping data for the frontsector's slope
			fslope->highz = line->frontsector->f_slope->highz = highest;
			fslope->lowz = line->frontsector->f_slope->lowz = lowest;
			
			// SRB2CBTODO: Get XY angle of a slope and then awesome physics! //        ESLOPE:
			//fslope->zangle = line->frontsector->f_slope->zangle = P_GetSlopezangle(line->frontsector, highvert, lowvert);
			//100*(ANG45/45);//R_PointToAngle2(direction.x, direction.y, origin.x, origin.y);
			// Get slope XY angle with secplane_t
			secplane_t *srcplane = Z_Calloc(sizeof(*srcplane), PU_LEVEL, NULL);
			// ZDoom secplane port!
			// secplane_t! woot!
			// ret = f_slope or c_slope
			srcplane->a = FLOAT_TO_FIXED (fslope->normalf.x); // cross[0]
			srcplane->b = FLOAT_TO_FIXED (fslope->normalf.y); // cross[1]
			srcplane->c = FLOAT_TO_FIXED (fslope->normalf.z); // cross[2]
			//plane->ic = FLOAT_TO_FIXED (1.f/cross[2]);
			srcplane->ic = DivScale32 (1, srcplane->c); // (1 << 32/srcplane->c)
#ifdef SLOPETHINGS // For setting thing-based slopes
			srcplane->d = -TMulScale16 (plane->a, x,
										plane->b, y,
										plane->c, z);
#endif
			//srcheight = isceiling ? sec->GetPlaneTexZ(sector_t::floor) : sec->GetPlaneTexZ(sector_t::ceiling);
			//destheight = isceiling ? refsec->GetPlaneTexZ(sector_t::floor) : refsec->GetPlaneTexZ(sector_t::ceiling);
			//P_GetZAtf(ret, v2.x, v2.y)
			// destheight takes the destination height used in dz 
			srcplane->d = -TMulScale16 (srcplane->a, line->v1->x,
										srcplane->b, line->v1->y,
										srcplane->c, line->backsector->floorheight);
			
			// Sync the secplane!
			fslope->secplane = line->frontsector->f_slope->secplane = *srcplane;
			
		}
	}
}
#endif
#endif



//===========================================================================
//
// P_SpawnSlopeMakers
//
//===========================================================================
#if 0
void P_SpawnSlopeMakers (FMapThing *firstmt, FMapThing *lastmt)
{
	FMapThing *mt;
	
	for (mt = firstmt; mt < lastmt; ++mt)
	{
		if ((mt->type >= THING_SlopeFloorPointLine &&
			 mt->type <= THING_SetCeilingSlope) ||
			mt->type==THING_VavoomFloor || mt->type==THING_VavoomCeiling)
		{
			fixed_t x, y, z;
			secplane_t *refplane;
			sector_t *sec;
			
			x = mt->x;
			y = mt->y;
			sec = P_PointInSector (x, y);
			if (mt->type & 1)
			{
				refplane = &sec->ceilingplane;
			}
			else
			{
				refplane = &sec->floorplane;
			}
			z = refplane->ZatPoint (x, y) + (mt->z);
			if (mt->type==THING_VavoomFloor || mt->type==THING_VavoomCeiling)
			{
				P_VavoomSlope(sec, mt->thingid, x, y, mt->z, mt->type & 1); 
			}
			else if (mt->type <= THING_SlopeCeilingPointLine)
			{
				P_SlopeLineToPoint (mt->args[0], x, y, z, mt->type & 1);
			}
			else
			{
				P_SetSlope (refplane, mt->type & 1, mt->angle, mt->args[0], x, y, z);
			}
			mt->type = 0;
		}
	}
	
	for (mt = firstmt; mt < lastmt; ++mt)
	{
		if (mt->type == THING_CopyFloorPlane ||
			mt->type == THING_CopyCeilingPlane)
		{
			P_CopyPlane (mt->args[0], mt->x, mt->y, mt->type & 1);
			mt->type = 0;
		}
	}
	
	P_SetSlopesFromVertexHeights(firstmt, lastmt);
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

//
// P_DistFromPlanef
//
float P_DistFromPlanef(const v3float_t *point, const v3float_t *pori, 
                       const v3float_t *pnormal)
{
   return (point->x - pori->x) * pnormal->x + 
          (point->y - pori->y) * pnormal->y +
          (point->z - pori->z) * pnormal->z;
}

// EOF
#endif // #ifdef ESLOPE

