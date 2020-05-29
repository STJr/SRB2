// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2001 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief special trick routines to make some SW tricks look OK with
///	HW rendering. This includes:
///	- deepwatereffect (e.g. tnt/map02)
///	- invisible staircase (e.g. eternal/map02)
///	- floating ceilings (e.g. eternal/map03)
///
///	It is not guaranteed that it looks identical to the SW mode,
///	but it looks in most of the cases far better than having
///	holes in the architecture, HOM, etc.
///
///	It fixes as well missing textures, which are replaced by either
///	a default texture or the midtexture.
///
///	words of notice:
///	pseudosectors, as mentioned in this file, are sectors where both
///	sidedefs point to the same sector. This expression is also used
///	for sectors which are enclosed by another sector but have no
///	correct sidedefs at all
///
///	if a vertex is inside a poly is determined by the angles between
///	this vertex and all angles on the linedefs (imagine walking along
///	a circle always facing a certain point inside/outside the circle;
///	if inside, angle have taken all values [0..pi), otherwise the
///	range was < pi/2

#include <math.h>
#include "../doomdef.h"
#include "../doomstat.h"

#ifdef HWRENDER
#include "hw_glob.h"
#include "hw_dll.h"
#include "../r_local.h"
#include "../i_system.h"

//
// add a line to a sectors list of lines
//
static void addLineToChain(sector_t *sector, line_t *line)
{
	linechain_t *thisElem = NULL, *nextElem;

	if (!sector)
		return;

	nextElem = sector->sectorLines;

	while (nextElem) // walk through chain
	{
		thisElem = nextElem;
		nextElem = thisElem->next;
	}

	// add a new element into the chain
	if (thisElem)
	{
		thisElem->next = malloc(sizeof (linechain_t));
		if (thisElem->next)
		{
			thisElem->next->line = line;
			thisElem->next->next = NULL;
		}
		else
		{
			I_Error("Out of memory in addLineToChain(.)\n");
		}
	}
	else // first element in chain
	{
		sector->sectorLines =  malloc(sizeof (linechain_t));
		if (sector->sectorLines)
		{
			sector->sectorLines->line = line;
			sector->sectorLines->next = NULL;
		}
		else
		{
			I_Error("Out of memory in addLineToChain(.)\n");
		}
	}
}

//
// We dont want a memory hole, do we?;-)
//
static void releaseLineChains(void)
{
	linechain_t *thisElem, *nextElem;
	sector_t *sector;
	size_t i;

	for (i = 0; i < numsectors; i++)
	{
		sector = &sectors[i];
		nextElem = sector->sectorLines;

		while (nextElem)
		{
			thisElem = nextElem;
			nextElem = thisElem->next;
			free(thisElem);
		}

		sector->sectorLines = NULL;
	}
}

//
// check if a pseudo sector is valid by checking all its linedefs
//
static boolean isPSectorValid(sector_t *sector)
{
	linechain_t *thisElem, *nextElem;

	if (!sector->pseudoSector) // check only pseudosectors, others dont care
	{
#ifdef PARANOIA
		CONS_Debug(DBG_RENDER, "Alert! non-pseudosector fed to isPSectorClosed()\n");
#endif
		return false;
	}

	nextElem = sector->sectorLines;

	while (nextElem)
	{
		thisElem = nextElem;
		nextElem = thisElem->next;
		if (thisElem->line->frontsector != thisElem->line->backsector)
			return false;
	}
	return true;
}

//
// angles are always phiMax-phiMin [0...2\pi)
//
FUNCMATH static double phiDiff(double phiMin, double phiMax)
{
	double result;

	result = phiMax-phiMin;

	if (result < 0.0l)
		result += 2.0l*M_PIl;

	return result;
}

//
// sort phi's so that enclosed angle < \pi
//
static void sortPhi(double phi1, double phi2, double *phiMin, double *phiMax)
{
	if (phiDiff(phi1, phi2) < M_PIl)
	{
		*phiMin = phi1;
		*phiMax = phi2;
	}
	else
	{
		*phiMin = phi2;
		*phiMax = phi1;
	}
}

//
// return if angle(phi1, phi2) is bigger than \pi
// if so, the vertex lies inside the poly
//
FUNCMATH static boolean biggerThanPi(double phi1, double phi2)
{
	if (phiDiff(phi1, phi2) > M_PIl)
		return true;

	return false;
}

#define DELTAPHI (M_PIl/100.0l) // some small phi << \pi

//
// calculate bounds for minimum angle
//
static void phiBounds(double phi1, double phi2, double *phiMin, double *phiMax)
{
	double phi1Tmp, phi2Tmp;
	double psi1, psi2, psi3, psi4, psi5, psi6, psi7; // for optimization

	sortPhi(phi1, phi2, &phi1Tmp, &phi2Tmp);
	phi1 = phi1Tmp;
	phi2 = phi2Tmp;

	// check start condition
	if (*phiMin > M_PIl || *phiMax > M_PIl)
	{
		*phiMin = phi1;
		*phiMax = phi2;
		return;
	}

	// 6 cases:
	// new angles inbetween phiMin, phiMax -> forget it
	// new angles enclose phiMin -> set phiMin
	// new angles enclose phiMax -> set phiMax
	// new angles completely outside phiMin, phiMax -> leave largest area free
	// new angles close the range completely!
	// new angles enlarges range on both sides

	psi1 = phiDiff(*phiMin, phi1);
	psi2 = phiDiff(*phiMin, phi2);
	psi3 = phiDiff(*phiMax, phi1);
	psi4 = phiDiff(*phiMax, phi2);
	psi5 = phiDiff(*phiMin, *phiMax);
	psi6 = (double)(2.0l*M_PIl - psi5); // phiDiff(*phiMax, *phiMin);
	psi7 = (double)(2.0l*M_PIl - psi2); // phiDiff(phi2, *phiMin);

	// case 1 & 5!
	if ((psi1 <= psi5) && (psi2 <= psi5))
	{
		if (psi1 <= psi2) // case 1
		{
			return;
		}
		else // case 5
		{
			// create some artificial interval here not to get into numerical trouble
			// in fact we know now the sector is completely enclosed -> base for computational optimization
			*phiMax = 0.0l;
			*phiMin = DELTAPHI;
			return;
		}
	}

	// case 2
	if ((psi1 >= psi5) && (psi2 <= psi5))
	{
		*phiMin = phi1;
		return;
	}

	// case 3
	if ((psi3 >= psi6) && (psi4 <= psi6))
	{
		*phiMax = phi2;
		return;
	}

	// case 4 & 6
#ifdef PARANOIA
	if ((psi3 <= psi6) && (psi4 <= psi6)) // FIXME: isn't this case implicitly true anyway??
#endif
	{
		if (psi3 <= psi4) //case 4
		{
			if (psi3 >= psi7)
			{
				*phiMin = phi1;
				return;
			}
			else
			{
				*phiMax = phi2;
				return;
			}
		}
		else // case 6
		{
			*phiMin = phi1;
			*phiMax = phi2;
			return;
		}
	}

#ifdef PARANOIA
	CONS_Debug(DBG_RENDER, "phiMin = %f, phiMax = %f, phi1 = %f, phi2 = %f\n", *phiMin, *phiMax, phi1, phi2);
	I_Error("phiBounds() out of range!\n");
#endif
}

//
// Check if a vertex lies inside a sector
// This works for "well-behaved" convex polygons
// If we need it mathematically correct, we need to sort the
// linedefs first so we have them in a row, then walk along the linedefs,
// but this is a bit overdone
//
static inline boolean isVertexInside(vertex_t *vertex, sector_t *sector)
{
	double xa, ya, xe, ye;
	linechain_t *chain;
	double phiMin, phiMax;
	double phi1, phi2;

	chain = sector->sectorLines;
	phiMin = phiMax = 10.0l*M_PIl; // some value > \pi

	while (chain)
	{
		// start and end vertex
		xa = (double)chain->line->v1->x - (double)vertex->x;
		ya = (double)chain->line->v1->y - (double)vertex->y;
		xe = (double)chain->line->v2->x - (double)vertex->x;
		ye = (double)chain->line->v2->y - (double)vertex->y;

		// angle phi of connection between the vertices and the x-axis
		phi1 = atan2(ya, xa);
		phi2 = atan2(ye, xe);

		// if we have just started, we can have to create start bounds for phi

		phiBounds(phi1, phi2, &phiMin, &phiMax);
		chain = chain->next;
	}

	return biggerThanPi(phiMin, phiMax);
}


#define MAXSTACK 256 // Not more than 256 polys in each other?
//
// generate a list of sectors which enclose the given sector
//
static void generateStacklist(sector_t *thisSector)
{
	size_t i, stackCnt = 0;
	sector_t *locStacklist[MAXSTACK];
	sector_t *checkSector;

	for (i = 0; i < numsectors; i++)
	{
		checkSector = &sectors[i];

		if (checkSector == thisSector) // dont check self
			continue;

		// buggy sector?
		if (!thisSector->sectorLines)
			continue;

		// check if an arbitrary vertex of thisSector lies inside the checkSector
		if (isVertexInside(thisSector->sectorLines->line->v1, checkSector))
		{
			// if so, the thisSector lies inside the checkSector
			locStacklist[stackCnt] = checkSector;
			stackCnt++;

			if (MAXSTACK-1 == stackCnt) // beware of the SIGSEGV! and consider terminating NULL!
				break;
		}
	}

	thisSector->stackList = malloc(sizeof (sector_t *) * (stackCnt+1));
	if (NULL == thisSector->stackList)
	{
		I_Error("Out of memory error in generateStacklist()");
	}

	locStacklist[stackCnt] = NULL; // terminating NULL

	memcpy(thisSector->stackList, locStacklist, sizeof (sector_t *) * (stackCnt+1));
}

//
// Bubble sort the stacklist with rising lineoutlengths
//
static void sortStacklist(sector_t *sector)
{
	sector_t **list;
	sector_t *sec1, *sec2;
	boolean finished;
	size_t i;

	list = sector->stackList;
	finished = false;

	if (!*list)
		return; // nothing to sort

	while (!finished)
	{
		i = 0;
		finished = true;

		while (*(list+i+1))
		{
			sec1 = *(list+i);
			sec2 = *(list+i+1);

			if (sec1->lineoutLength > sec2->lineoutLength)
			{
				*(list+i) = sec2;
				*(list+i+1) = sec1;
				finished = false;
			}
			i++;
		}
	}
}

//
// length of a line in euclidian sense
//
static double lineLength(line_t *line)
{
	double dx, dy, length;

	dx = (double) line->v1->x - (double) line->v2->x;
	dy = (double) line->v1->y - (double) line->v2->y;

	length = hypot(dx, dy);

	return length;
}


//
// length of the sector lineout
//
static double calcLineoutLength(sector_t *sector)
{
	linechain_t *chain;
	double length = 0.0L;
	chain = sector->sectorLines;

	while (chain) // sum up lengths of all lines
	{
		length += lineLength(chain->line);
		chain = chain->next;
	}
	return length;
}

//
// Calculate length of the sectors lineout
//
static void calcLineouts(sector_t *sector)
{
	size_t secCount = 0;
	sector_t *encSector = *(sector->stackList);

	while (encSector)
	{
		if (encSector->lineoutLength < 0.0L) // if length has not yet been calculated
		{
			encSector->lineoutLength = calcLineoutLength(encSector);
		}

		secCount++;
		encSector = *((sector->stackList) + secCount);
	}
}

//
// Free Stacklists of all sectors
//
static void freeStacklists(void)
{
	size_t i;

	for (i = 0; i < numsectors; i++)
	{
		if (sectors[i].stackList)
		{
			free(sectors[i].stackList);
			sectors[i].stackList = NULL;
		}
	}
}

//
// if more than half of the toptextures are missing
//
static boolean areToptexturesMissing(sector_t *thisSector)
{
	linechain_t *thisElem, *nextElem = thisSector->sectorLines;
	sector_t *frontSector, *backSector;
	size_t nomiss = 0;
	side_t *sidel, *sider;

	while (nextElem) // walk through chain
	{
		thisElem = nextElem;
		nextElem = thisElem->next;

		frontSector = thisElem->line->frontsector;
		backSector  = thisElem->line->backsector;

		if (frontSector == backSector) // skip damn renderer tricks here
			continue;

		if (!frontSector || !backSector)
			continue;

		sider = &sides[thisElem->line->sidenum[0]];
		sidel = &sides[thisElem->line->sidenum[1]];

		if (backSector->ceilingheight < frontSector->ceilingheight)
		{
			if (sider->toptexture != 0)
			{
				nomiss++;
				break; // we can stop here if decision criterium is ==0
			}
		}
		else if (backSector->ceilingheight > frontSector->ceilingheight)
		{
			if (sidel->toptexture != 0)
			{
				nomiss++;
				break; // we can stop here if decision criterium is ==0
			}
		}
	}

	return nomiss == 0;
}

//
// are more textures missing than present?
//
static boolean areBottomtexturesMissing(sector_t *thisSector)
{
	linechain_t *thisElem, *nextElem = thisSector->sectorLines;
	sector_t *frontSector, *backSector;
	size_t nomiss = 0;
	side_t *sidel, *sider;

	while (nextElem) // walk through chain
	{
		thisElem = nextElem;
		nextElem = thisElem->next;

		frontSector = thisElem->line->frontsector;
		backSector  = thisElem->line->backsector;

		if (frontSector == backSector) // skip damn renderer tricks here
			continue;

		if (!frontSector || !backSector)
			continue;

		sider = &sides[thisElem->line->sidenum[0]];
		sidel = &sides[thisElem->line->sidenum[1]];

		if (backSector->floorheight > frontSector->floorheight)
		{
			if (sider->bottomtexture != 0)
			{
				nomiss++;
				break; // we can stop here if decision criterium is ==0
			}
		}

		else if (backSector->floorheight < frontSector->floorheight)
		{
			if (sidel->bottomtexture != 0)
			{
				nomiss++;
				break; // we can stop here if decision criterium is ==0
			}
		}
	}

	//    return missing >= nomiss;
	return nomiss == 0;
}

//
// check if no adjacent sector has same ceiling height
//
static boolean isCeilingFloating(sector_t *thisSector)
{
	sector_t *adjSector, *refSector = NULL, *frontSector, *backSector;
	linechain_t *thisElem, *nextElem;

	if (!thisSector)
		return false;

	nextElem = thisSector->sectorLines;

	while (nextElem) // walk through chain
	{
		thisElem = nextElem;
		nextElem = thisElem->next;

		frontSector = thisElem->line->frontsector;
		backSector  = thisElem->line->backsector;

		if (frontSector == thisSector)
			adjSector = backSector;
		else
			adjSector = frontSector;

		if (!adjSector) // assume floating sectors have surrounding sectors
			return false;

		if (adjSector->c_slope) // Don't bother with slopes
			return false;

		if (!refSector)
		{
			refSector = adjSector;
			continue;
		}

		// if adjacent sector has same height or more than one adjacent sector exists -> stop
		if (thisSector->ceilingheight == adjSector->ceilingheight || refSector != adjSector)
			return false;
	}

	// now check for walltextures
	if (!areToptexturesMissing(thisSector))
		return false;

	return true;
}

//
// check if no adjacent sector has same ceiling height
// FIXME: throw that together with isCeilingFloating??
//
static boolean isFloorFloating(sector_t *thisSector)
{
	sector_t *adjSector, *refSector = NULL, *frontSector, *backSector;
	linechain_t *thisElem, *nextElem;

	if (!thisSector)
		return false;

	nextElem = thisSector->sectorLines;

	while (nextElem) // walk through chain
	{
		thisElem = nextElem;
		nextElem = thisElem->next;

		frontSector = thisElem->line->frontsector;
		backSector  = thisElem->line->backsector;

		if (frontSector == thisSector)
			adjSector = backSector;
		else
			adjSector = frontSector;

		if (!adjSector) // assume floating sectors have surrounding sectors
			return false;

		if (adjSector->f_slope) // Don't bother with slopes
			return false;

		if (!refSector)
		{
			refSector = adjSector;
			continue;
		}

		// if adjacent sector has same height or more than one adjacent sector exists -> stop
		if (thisSector->floorheight == adjSector->floorheight || refSector != adjSector)
			return false;
	}

	// now check for walltextures
	if (!areBottomtexturesMissing(thisSector))
		return false;

	return true;
}

//
// estimate ceilingheight according to height of adjacent sector
//
static fixed_t estimateCeilHeight(sector_t *thisSector)
{
	sector_t *adjSector;

	if (!thisSector || !thisSector->sectorLines || !thisSector->sectorLines->line)
		return 0;

	adjSector = thisSector->sectorLines->line->frontsector;
	if (adjSector == thisSector)
		adjSector = thisSector->sectorLines->line->backsector;

	if (!adjSector)
		return 0;

	return adjSector->ceilingheight;
}

//
// estimate ceilingheight according to height of adjacent sector
//
static fixed_t estimateFloorHeight(sector_t *thisSector)
{
	sector_t *adjSector;

	if (!thisSector || !thisSector->sectorLines || !thisSector->sectorLines->line)
		return 0;

	adjSector = thisSector->sectorLines->line->frontsector;
	if (adjSector == thisSector)
		adjSector = thisSector->sectorLines->line->backsector;

	if (!adjSector)
		return 0;

	return adjSector->floorheight;
}

#define CORRECT_FLOAT_EXPERIMENTAL

// --------------------------------------------------------------------------
// Some levels have missing sidedefs, which produces HOM, so lets try to compensate for that
// and some levels have deep water trick, invisible staircases etc.
// --------------------------------------------------------------------------
// FIXME: put some nice default texture in legacy.dat and use it
void HWR_CorrectSWTricks(void)
{
	size_t i;
	size_t k;
	line_t *ld;
	side_t *sidel = NULL, *sider;
	sector_t *secl, *secr;
	sector_t **sectorList;
	sector_t *outSector;

	if ((0 == cv_grcorrecttricks.value))
		return;

	// determine lines for sectors
	for (i = 0; i < numlines; i++)
	{
		ld = &lines[i];
		secr = ld->frontsector;
		secl = ld->backsector;

		if (secr == secl)
		{
			secr->pseudoSector = true; // special renderer trick?
			addLineToChain(secr, ld);
		}
		else
		{
			addLineToChain(secr, ld);
			addLineToChain(secl, ld);
		}
	}

	// preprocessing
	for (i = 0; i < numsectors; i++)
	{
		sector_t *checkSector;

		checkSector = &sectors[i];

		// identify real pseudosectors first
		if (checkSector->pseudoSector)
		{
			if (!isPSectorValid(checkSector)) // drop invalid pseudo sectors
			{
				checkSector->pseudoSector = false;
			}
		}

		// determine enclosing sectors for pseudosectors ... used later
		if (checkSector->pseudoSector)
		{
			generateStacklist(checkSector);
			calcLineouts(checkSector);
			sortStacklist(checkSector);
		}
	}

	// set virtual floor heights for pseudo sectors
	// required for deep water effect e.g.
	for (i = 0; i < numsectors; i++)
	{
		if (sectors[i].pseudoSector)
		{
			sectorList = sectors[i].stackList;
			k = 0;
			while (*(sectorList+k))
			{
				outSector = *(sectorList+k);
				if (!outSector->pseudoSector)
				{
					sectors[i].virtualFloorheight = outSector->floorheight;
					sectors[i].virtualCeilingheight = outSector->ceilingheight;
					break;
				}
				k++;
			}
			if (*(sectorList+k) == NULL) // sorry, did not work :(
			{
				sectors[i].virtualFloorheight = sectors[i].floorheight;
				sectors[i].virtualCeilingheight = sectors[i].ceilingheight;
			}
		}
	}
#ifdef CORRECT_FLOAT_EXPERIMENTAL
	// correct ceiling/floor heights of totally floating sectors
	for (i = 0; i < numsectors; i++)
	{
		sector_t *floatSector;

		floatSector = &sectors[i];

		// correct height of floating sectors
		if (isCeilingFloating(floatSector))
		{
			floatSector->virtualCeilingheight = estimateCeilHeight(floatSector);
			floatSector->virtualCeiling = true;
		}
		if (isFloorFloating(floatSector))
		{
			floatSector->virtualFloorheight = estimateFloorHeight(floatSector);
			floatSector->virtualFloor = true;
		}
	}
#endif

	// now for the missing textures
	for (i = 0; i < numlines; i++)
	{
		ld = &lines[i];
		sider = &sides[ld->sidenum[0]];
		if (ld->sidenum[1] != 0xffff)
			sidel = &sides[ld->sidenum[1]];

		secr = ld->frontsector;
		secl = ld->backsector;

		if (secr == secl) // special renderer trick
			continue; // we cant correct missing textures here

		if (secl) // only if there is a backsector
		{
			if (secr->pseudoSector || secl->pseudoSector)
				continue;
			if (!secr->virtualFloor && !secl->virtualFloor)
			{
				if (secl->floorheight > secr->floorheight)
				{
					// now check if r-sidedef is correct
					if (sider->bottomtexture == 0)
					{
						if (sider->midtexture == 0)
							sider->bottomtexture = 0; // Tails // More redwall sky shenanigans
						else
							sider->bottomtexture = sider->midtexture;
					}
				}
				else if (secl->floorheight < secr->floorheight)
				{
					// now check if l-sidedef is correct
					if (sidel->bottomtexture == 0)
					{
						if (sidel->midtexture == 0)
							sidel->bottomtexture = 0; // Tails // More redwall sky shenanigans
						else
							sidel->bottomtexture = sidel->midtexture;
					}
				}
			}

			if (!secr->virtualCeiling && !secl->virtualCeiling)
			{
					if (secl->ceilingheight < secr->ceilingheight)
				{
					// now check if r-sidedef is correct
					if (sider->toptexture == 0)
					{
						if (sider->midtexture == 0)
							sider->toptexture = 0; // Tails // When this was REDWALL it was causing issues in the sky sometimes
						else
							sider->toptexture = sider->midtexture;
					}
				}
				else if (secl->ceilingheight > secr->ceilingheight)
				{
					// now check if l-sidedef is correct
					if (sidel->toptexture == 0)
					{
						if (sidel->midtexture == 0)
							sidel->toptexture = 0; // Tails // When this was REDWALL it was causing issues in the sky sometimes
						else
							sidel->toptexture = sidel->midtexture;
					}
				}
			}
		} // if (NULL != secl)
	} // for (i = 0; i < numlines; i++)

	// release all linechains
	releaseLineChains();
	freeStacklists();
}

#endif // HWRENDER
