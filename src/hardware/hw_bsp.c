// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_bsp.c
/// \brief convert SRB2 map

#include "../doomdef.h"
#include "../doomstat.h"
#ifdef HWRENDER
#include "hw_glob.h"
#include "../r_local.h"
#include "../z_zone.h"
#include "../console.h"
#include "../v_video.h"
#include "../m_menu.h"
#include "../i_system.h"
#include "../m_argv.h"
#include "../i_video.h"
#include "../w_wad.h"
#include "../p_setup.h" // levelfadecol

// --------------------------------------------------------------------------
// This is global data for planes rendering
// --------------------------------------------------------------------------

extrasubsector_t *extrasubsectors = NULL;

// newsubsectors are subsectors without segs, added for the plane polygons
#define NEWSUBSECTORS 50
static size_t totsubsectors;
size_t addsubsector;

typedef struct
{
	float x, y;
	float dx, dy;
} fdivline_t;

// ==========================================================================
//                                    FLOOR & CEILING CONVEX POLYS GENERATION
// ==========================================================================

//debug counters
static INT32 nobackpoly = 0;
static INT32 skipcut = 0;
static INT32 totalsubsecpolys = 0;

// --------------------------------------------------------------------------
// Polygon fast alloc / free
// --------------------------------------------------------------------------
//hurdler: quick fix for those who wants to play with larger wad

#define ZPLANALLOC
#ifndef ZPLANALLOC
//#define POLYPOOLSIZE 1024000 // may be much over what is needed
/// \todo check out how much is used
static size_t POLYPOOLSIZE = 1024000;

static UINT8 *gl_polypool = NULL;
static UINT8 *gl_ppcurrent;
static size_t gl_ppfree;
#endif

// only between levels, clear poly pool
static void HWR_ClearPolys(void)
{
#ifndef ZPLANALLOC
	gl_ppcurrent = gl_polypool;
	gl_ppfree = POLYPOOLSIZE;
#endif
}

// allocate  pool for fast alloc of polys
void HWR_InitPolyPool(void)
{
#ifndef ZPLANALLOC
	INT32 pnum;

	//hurdler: quick fix for those who wants to play with larger wad
	if ((pnum = M_CheckParm("-polypoolsize")))
		POLYPOOLSIZE = atoi(myargv[pnum+1])*1024; // (in kb)

	CONS_Debug(DBG_RENDER, "HWR_InitPolyPool(): allocating %d bytes\n", POLYPOOLSIZE);
	gl_polypool = malloc(POLYPOOLSIZE);
	if (!gl_polypool)
		I_Error("HWR_InitPolyPool(): couldn't malloc polypool\n");
	HWR_ClearPolys();
#endif
}

void HWR_FreePolyPool(void)
{
#ifndef ZPLANALLOC
	if (gl_polypool)
		free(gl_polypool);
	gl_polypool = NULL;
#endif
}

static poly_t *HWR_AllocPoly(INT32 numpts)
{
	poly_t *p;
	size_t size = sizeof (poly_t) + sizeof (polyvertex_t) * numpts;
#ifdef ZPLANALLOC
	p = Z_Malloc(size, PU_HWRPLANE, NULL);
#else
#ifdef PARANOIA
	if (!gl_polypool)
		I_Error("Used gl_polypool without init!\n");
	if (!gl_ppcurrent)
		I_Error("gl_ppcurrent == NULL!\n");
#endif

	if (gl_ppfree < size)
		I_Error("HWR_AllocPoly(): no more memory %u bytes left, %u bytes needed\n\n%s\n",
		        gl_ppfree, size, "You can try the param -polypoolsize 2048 (or higher if needed)");

	p = (poly_t *)gl_ppcurrent;
	gl_ppcurrent += size;
	gl_ppfree -= size;
#endif
	p->numpts = numpts;
	return p;
}

static polyvertex_t *HWR_AllocVertex(void)
{
	polyvertex_t *p;
	size_t size = sizeof (polyvertex_t);
#ifdef ZPLANALLOC
	p = Z_Malloc(size, PU_HWRPLANE, NULL);
#else
	if (gl_ppfree < size)
		I_Error("HWR_AllocVertex(): no more memory %u bytes left, %u bytes needed\n\n%s\n",
		        gl_ppfree, size, "You can try the param -polypoolsize 2048 (or higher if needed)");

	p = (polyvertex_t *)gl_ppcurrent;
	gl_ppcurrent += size;
	gl_ppfree -= size;
#endif
	return p;
}

/// \todo polygons should be freed in reverse order for efficiency,
/// for now don't free because it doesn't free in reverse order
static void HWR_FreePoly(poly_t *poly)
{
#ifdef ZPLANALLOC
	Z_Free(poly);
#else
	const size_t size = sizeof (poly_t) + sizeof (polyvertex_t) * poly->numpts;
	memset(poly, 0x00, size);
	//mempoly -= polysize;
#endif
}


// Return interception along bsp line,
// with the polygon segment
//
static float bspfrac;
static polyvertex_t *fracdivline(fdivline_t *bsp, polyvertex_t *v1,
	polyvertex_t *v2)
{
	static polyvertex_t pt;
	double frac;
	double num;
	double den;
	double v1x,v1y,v1dx,v1dy;
	double v2x,v2y,v2dx,v2dy;

	// a segment of a polygon
	v1x  = v1->x;
	v1y  = v1->y;
	v1dx = v2->x - v1->x;
	v1dy = v2->y - v1->y;

	// the bsp partition line
	v2x  = bsp->x;
	v2y  = bsp->y;
	v2dx = bsp->dx;
	v2dy = bsp->dy;

	den = v2dy*v1dx - v2dx*v1dy;
	if (fabsf((float)den) < 1.0E-36f) // avoid checking exactly for 0.0
		return NULL;       // parallel

	// first check the frac along the polygon segment,
	// (do not accept hit with the extensions)
	num = (v2x - v1x)*v2dy + (v1y - v2y)*v2dx;
	frac = num / den;
	if (frac < 0.0l || frac > 1.0l)
		return NULL;

	// now get the frac along the BSP line
	// which is useful to determine what is left, what is right
	num = (v2x - v1x)*v1dy + (v1y - v2y)*v1dx;
	frac = num / den;
	bspfrac = (float)frac;


	// find the interception point along the partition line
	pt.x = (float)(v2x + v2dx*frac);
	pt.y = (float)(v2y + v2dy*frac);

	return &pt;
}

// if two vertice coords have a x and/or y difference
// of less or equal than 1 FRACUNIT, they are considered the same
// point. Note: hardcoded value, 1.0f could be anything else.
static boolean SameVertice (polyvertex_t *p1, polyvertex_t *p2)
{
#if 0
	float diff;
	diff = p2->x - p1->x;
	if (diff < -1.5f || diff > 1.5f)
		return false;
	diff = p2->y - p1->y;
	if (diff < -1.5f || diff > 1.5f)
		return false;
#elif 0
	if (p1->x != p2->x)
		return false;
	if (p1->y != p2->y)
		return false;
#elif 0
	if (fabsf( p2->x - p1->x ) > 1.0E-36f )
		return false;
	if (fabsf( p2->y - p1->y ) > 1.0E-36f )
		return false;
#else
#define  DIVLINE_VERTEX_DIFF   0.45f
	float ep = DIVLINE_VERTEX_DIFF;
	if (fabsf( p2->x - p1->x ) > ep )
		return false;
	if (fabsf( p2->y - p1->y ) > ep )
		return false;
#endif
	// p1 and p2 are considered the same vertex
	return true;
}


// split a _CONVEX_ polygon in two convex polygons
// outputs:
//   frontpoly : polygon on right side of bsp line
//   backpoly  : polygon on left side
//
static void SplitPoly (fdivline_t *bsp,         //splitting parametric line
                       poly_t *poly,            //the convex poly we split
                       poly_t **frontpoly,      //return one poly here
                       poly_t **backpoly)       //return the other here
{
	INT32      i,j;
	polyvertex_t *pv;

	INT32          ps = -1,pe = -1;
	INT32          nptfront,nptback;
	polyvertex_t vs = {0,0,0};
	polyvertex_t ve = {0,0,0};
	polyvertex_t lastpv = {0,0,0};
	float        fracs = 0.0f,frace = 0.0f; //used to tell which poly is on
	                                        // the front side of the bsp partition line
	INT32         psonline = 0, peonline = 0;

	for (i = 0; i < poly->numpts; i++)
	{
		j = i + 1;
		if (j == poly->numpts) j = 0;

		// start & end points
		pv = fracdivline(bsp, &poly->pts[i], &poly->pts[j]);

		if (pv == NULL)
			continue;

		if (ps < 0)
		{
			// first point
			ps = i;
			vs = *pv;
			fracs = bspfrac;
		}
		else
		{
			//the partition line traverse a junction between two segments
			// or the two points are so close, they can be considered as one
			// thus, don't accept, since split 2 must be another vertex
			if (SameVertice(pv, &lastpv))
			{
				if (pe < 0)
				{
					ps = i;
					psonline = 1;
				}
				else
				{
					pe = i;
					peonline = 1;
				}
			}
			else
			{
				if (pe < 0)
				{
					pe = i;
					ve = *pv;
					frace = bspfrac;
				}
				else
				{
				// a frac, not same vertice as last one
				// we already got pt2 so pt 2 is not on the line,
				// so we probably got back to the start point
				// which is on the line
					if (SameVertice(pv, &vs))
						psonline = 1;
					break;
				}
			}
		}

		// remember last point intercept to detect identical points
		lastpv = *pv;
	}

	// no split: the partition line is either parallel and
	// aligned with one of the poly segments, or the line is totally
	// out of the polygon and doesn't traverse it (happens if the bsp
	// is fooled by some trick where the sidedefs don't point to
	// the right sectors)
	if (ps < 0)
	{
		//I_Error("SplitPoly: did not split polygon (%d %d)\n"
		//        "debugpos %d",ps,pe,debugpos);

		// this eventually happens with 'broken' BSP's that accept
		// linedefs where each side point the same sector, that is:
		// the deep water effect with the original Doom

		/// \todo make sure front poly is to front of partition line?

		*frontpoly = poly;
		*backpoly = NULL;
		return;
	}

	if (pe < 0)
	{
		//I_Error("SplitPoly: only one point for split line (%d %d)", ps, pe);
		*frontpoly = poly;
		*backpoly = NULL;
		return;
	}
	if (pe <= ps)
		I_Error("SplitPoly: invalid splitting line (%d %d)", ps, pe);

	// number of points on each side, _not_ counting those
	// that may lie just one the line
	nptback  = pe - ps - peonline;
	nptfront = poly->numpts - peonline - psonline - nptback;

	if (nptback > 0)
		*backpoly = HWR_AllocPoly(2 + nptback);
	else
		*backpoly = NULL;
	if (nptfront > 0)
		*frontpoly = HWR_AllocPoly(2 + nptfront);
	else
		*frontpoly = NULL;

	// generate FRONT poly
	if (*frontpoly)
	{
		pv = (*frontpoly)->pts;
		*pv++ = vs;
		*pv++ = ve;
		i = pe;
		do
		{
			if (++i == poly->numpts)
				i = 0;
			*pv++ = poly->pts[i];
		} while (i != ps && --nptfront);
	}

	// generate BACK poly
	if (*backpoly)
	{
		pv = (*backpoly)->pts;
		*pv++ = ve;
		*pv++ = vs;
		i = ps;
		do
		{
			if (++i == poly->numpts)
				i = 0;
			*pv++ = poly->pts[i];
		} while (i != pe && --nptback);
	}

	// make sure frontpoly is the one on the 'right' side
	// of the partition line
	if (fracs > frace)
	{
		poly_t *swappoly;
		swappoly = *backpoly;
		*backpoly = *frontpoly;
		*frontpoly = swappoly;
	}

	HWR_FreePoly (poly);
}


// use each seg of the poly as a partition line, keep only the
// part of the convex poly to the front of the seg (that is,
// the part inside the sector), the part behind the seg, is
// the void space and is cut out
//
static poly_t *CutOutSubsecPoly(seg_t *lseg, INT32 count, poly_t *poly)
{
	INT32 i, j;

	polyvertex_t *pv;

	INT32 nump = 0, ps, pe;
	polyvertex_t vs = {0, 0, 0}, ve = {0, 0, 0},
		p1 = {0, 0, 0}, p2 = {0, 0, 0};
	float fracs = 0.0f;

	fdivline_t cutseg; // x, y, dx, dy as start of node_t struct

	poly_t *temppoly;

	// for each seg of the subsector
	for (; count--; lseg++)
	{
		line_t *line = lseg->linedef;

		if (lseg->glseg)
			continue;

		//x,y,dx,dy (like a divline)
		p1.x = FIXED_TO_FLOAT(lseg->side ? line->v2->x : line->v1->x);
		p1.y = FIXED_TO_FLOAT(lseg->side ? line->v2->y : line->v1->y);
		p2.x = FIXED_TO_FLOAT(lseg->side ? line->v1->x : line->v2->x);
		p2.y = FIXED_TO_FLOAT(lseg->side ? line->v1->y : line->v2->y);

		cutseg.x = p1.x;
		cutseg.y = p1.y;
		cutseg.dx = p2.x - p1.x;
		cutseg.dy = p2.y - p1.y;

		// see if it cuts the convex poly
		ps = -1;
		pe = -1;
		for (i = 0; i < poly->numpts; i++)
		{
			j = i + 1;
			if (j == poly->numpts)
				j = 0;

			pv = fracdivline(&cutseg, &poly->pts[i], &poly->pts[j]);

			if (pv == NULL)
				continue;

			if (ps < 0)
			{
				ps = i;
				vs = *pv;
				fracs = bspfrac;
			}
			else
			{
				//frac 1 on previous segment,
				//     0 on the next,
				//the split line goes through one of the convex poly
				// vertices, happens quite often since the convex
				// poly is already adjacent to the subsector segs
				// on most borders
				if (SameVertice(pv, &vs))
					continue;

				if (fracs <= bspfrac)
				{
					nump = 2 + poly->numpts - (i-ps);
					pe = ps;
					ps = i;
					ve = *pv;
				}
				else
				{
					nump = 2 + (i-ps);
					pe = i;
					ve = vs;
					vs = *pv;
				}
				//found 2nd point
				break;
			}
		}

		// there was a split
		if (ps >= 0)
		{
			//need 2 points
			if (pe >= 0)
			{
				// generate FRONT poly
				temppoly = HWR_AllocPoly(nump);
				pv = temppoly->pts;
				*pv++ = vs;
				*pv++ = ve;
				do
				{
					if (++ps == poly->numpts)
						ps = 0;
					*pv++ = poly->pts[ps];
				} while (ps != pe);
				HWR_FreePoly(poly);
				poly = temppoly;
			}
			//hmmm... maybe we should NOT accept this, but this happens
			// only when the cut is not needed it seems (when the cut
			// line is aligned to one of the borders of the poly, and
			// only some times..)
			else
				skipcut++;
			//    I_Error("CutOutPoly: only one point for split line (%d %d) %d", ps, pe, debugpos);
		}
	}
	return poly;
}

// At this point, the poly should be convex and the exact
// layout of the subsector, it is not always the case,
// so continue to cut off the poly into smaller parts with
// each seg of the subsector.
//
static inline void HWR_SubsecPoly(INT32 num, poly_t *poly)
{
	INT16 count;
	subsector_t *sub;
	seg_t *lseg;

	sub = &subsectors[num];
	count = sub->numlines;
	lseg = &segs[sub->firstline];

	if (poly)
	{
		poly = CutOutSubsecPoly (lseg,count,poly);
		totalsubsecpolys++;
		//extra data for this subsector
		extrasubsectors[num].planepoly = poly;
	}
}

// the bsp divline have not enouth presition
// search for the segs source of this divline
static inline void SearchDivline(node_t *bsp, fdivline_t *divline)
{
	divline->x = FIXED_TO_FLOAT(bsp->x);
	divline->y = FIXED_TO_FLOAT(bsp->y);
	divline->dx = FIXED_TO_FLOAT(bsp->dx);
	divline->dy = FIXED_TO_FLOAT(bsp->dy);
}

#ifdef HWR_LOADING_SCREEN
//Hurdler: implement a loading status
static size_t ls_count = 0;
static UINT8 ls_percent = 0;

static void loading_status(void)
{
	char s[16];
	int x, y;

	I_OsPolling();
	CON_Drawer();
	sprintf(s, "%d%%", (++ls_percent)<<1);
	x = BASEVIDWIDTH/2;
	y = BASEVIDHEIGHT/2;
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31); // Black background to match fade in effect
	//V_DrawPatchFill(W_CachePatchName("SRB2BACK",PU_CACHE)); // SRB2 background, ehhh too bright.
	M_DrawTextBox(x-58, y-8, 13, 1);
	V_DrawString(x-50, y, V_YELLOWMAP, "Loading...");
	V_DrawRightAlignedString(x+50, y, V_YELLOWMAP, s);

	// Is this really necessary at this point..?
	V_DrawCenteredString(BASEVIDWIDTH/2, 40, V_YELLOWMAP, "OPENGL MODE IS INCOMPLETE AND MAY");
	V_DrawCenteredString(BASEVIDWIDTH/2, 50, V_YELLOWMAP, "NOT DISPLAY SOME SURFACES.");
	V_DrawCenteredString(BASEVIDWIDTH/2, 70, V_YELLOWMAP, "USE AT SONIC'S RISK.");

	I_UpdateNoVsync();
}
#endif

// poly : the convex polygon that encloses all child subsectors
static void WalkBSPNode(INT32 bspnum, poly_t *poly, UINT16 *leafnode, fixed_t *bbox)
{
	node_t *bsp;
	poly_t *backpoly, *frontpoly;
	fdivline_t fdivline;
	polyvertex_t *pt;
	INT32 i;

	// Found a subsector?
	if (bspnum & NF_SUBSECTOR)
	{
		if (bspnum == -1)
		{
			// BP: i think this code is useless and wrong because
			// - bspnum==-1 happens only when numsubsectors == 0
			// - it can't happens in bsp recursive call since bspnum is a INT32 and children is UINT16
			// - the BSP is complet !! (there just can have subsector without segs) (i am not sure of this point)

			// do we have a valid polygon ?
			if (poly && poly->numpts > 2)
			{
				CONS_Debug(DBG_RENDER, "Adding a new subsector\n");
				if (addsubsector == numsubsectors + NEWSUBSECTORS)
					I_Error("WalkBSPNode: not enough addsubsectors\n");
				else if (addsubsector > 0x7fff)
					I_Error("WalkBSPNode: addsubsector > 0x7fff\n");
				*leafnode = (UINT16)((UINT16)addsubsector | NF_SUBSECTOR);
				extrasubsectors[addsubsector].planepoly = poly;
				addsubsector++;
			}

			//add subsectors without segs here?
			//HWR_SubsecPoly(0, NULL);
		}
		else
		{
			HWR_SubsecPoly(bspnum & ~NF_SUBSECTOR, poly);

			//Hurdler: implement a loading status
#ifdef HWR_LOADING_SCREEN
			if (ls_count-- <= 0)
			{
				ls_count = numsubsectors/50;
				loading_status();
			}
#endif
		}
		M_ClearBox(bbox);
		poly = extrasubsectors[bspnum & ~NF_SUBSECTOR].planepoly;

		for (i = 0, pt = poly->pts; i < poly->numpts; i++,pt++)
			M_AddToBox(bbox, FLOAT_TO_FIXED(pt->x), FLOAT_TO_FIXED(pt->y));

		return;
	}

	bsp = &nodes[bspnum];
	SearchDivline(bsp, &fdivline);
	SplitPoly(&fdivline, poly, &frontpoly, &backpoly);
	poly = NULL;

	//debug
	if (!backpoly)
		nobackpoly++;

	// Recursively divide front space.
	if (frontpoly)
	{
		WalkBSPNode(bsp->children[0], frontpoly, &bsp->children[0],bsp->bbox[0]);

		// copy child bbox
		M_Memcpy(bbox, bsp->bbox[0], 4*sizeof (fixed_t));
	}
	else
		I_Error("WalkBSPNode: no front poly?");

	// Recursively divide back space.
	if (backpoly)
	{
		// Correct back bbox to include floor/ceiling convex polygon
		WalkBSPNode(bsp->children[1], backpoly, &bsp->children[1], bsp->bbox[1]);

		// enlarge bbox with second child
		M_AddToBox(bbox, bsp->bbox[1][BOXLEFT  ],
		                 bsp->bbox[1][BOXTOP   ]);
		M_AddToBox(bbox, bsp->bbox[1][BOXRIGHT ],
		                 bsp->bbox[1][BOXBOTTOM]);
	}
}

// FIXME: use Z_Malloc() STATIC ?
void HWR_FreeExtraSubsectors(void)
{
	if (extrasubsectors)
		free(extrasubsectors);
	extrasubsectors = NULL;
}

#define MAXDIST 1.5f
// BP: can't move vertex: DON'T change polygon geometry! (convex)
//#define MOVEVERTEX
static boolean PointInSeg(polyvertex_t *a,polyvertex_t *v1,polyvertex_t *v2)
{
	register float ax,ay,bx,by,cx,cy,d,norm;
	register polyvertex_t *p;

	// check bbox of the seg first
	if (v1->x > v2->x)
	{
		p = v1;
		v1 = v2;
		v2 = p;
	}

	if (a->x < v1->x-MAXDIST || a->x > v2->x+MAXDIST)
		return false;

	if (v1->y > v2->y)
	{
		p = v1;
		v1 = v2;
		v2 = p;
	}
	if (a->y < v1->y-MAXDIST || a->y > v2->y+MAXDIST)
		return false;

	// v1 = origine
	ax= v2->x-v1->x;
	ay= v2->y-v1->y;
	norm = (float)hypot(ax, ay);
	ax /= norm;
	ay /= norm;
	bx = a->x-v1->x;
	by = a->y-v1->y;
	//d = a.b
	d =ax*bx+ay*by;
	// bound of the seg
	if (d < 0 || d > norm)
		return false;
	//c = d.1a-b
	cx = ax*d-bx;
	cy = ay*d-by;
#ifdef MOVEVERTEX
	if (cx*cx+cy*cy <= MAXDIST*MAXDIST)
	{
		// ajust a little the point position
		a->x = ax*d+v1->x;
		a->y = ay*d+v1->y;
		// anyway the correction is not enouth
		return true;
	}
	return false;
#else
	return cx*cx+cy*cy <= MAXDIST*MAXDIST;
#endif
}

static INT32 numsplitpoly;

static void SearchSegInBSP(INT32 bspnum,polyvertex_t *p,poly_t *poly)
{
	poly_t  *q;
	INT32     j,k;

	if (bspnum & NF_SUBSECTOR)
	{
		if (bspnum != -1)
		{
			bspnum &= ~NF_SUBSECTOR;
			q = extrasubsectors[bspnum].planepoly;
			if (poly == q || !q)
				return;
			for (j = 0; j < q->numpts; j++)
			{
				k = j+1;
				if (k == q->numpts) k = 0;
				if (!SameVertice(p, &q->pts[j])
					&& !SameVertice(p, &q->pts[k])
					&& PointInSeg(p, &q->pts[j],
						&q->pts[k]))
				{
					poly_t *newpoly = HWR_AllocPoly(q->numpts+1);
					INT32 n;

					for (n = 0; n <= j; n++)
						newpoly->pts[n] = q->pts[n];
					newpoly->pts[k] = *p;
					for (n = k+1; n < newpoly->numpts; n++)
						newpoly->pts[n] = q->pts[n-1];
					numsplitpoly++;
					extrasubsectors[bspnum].planepoly =
						newpoly;
					HWR_FreePoly(q);
					return;
				}
			}
		}
		return;
	}

	if ((FIXED_TO_FLOAT(nodes[bspnum].bbox[0][BOXBOTTOM])-MAXDIST <= p->y) &&
	    (FIXED_TO_FLOAT(nodes[bspnum].bbox[0][BOXTOP   ])+MAXDIST >= p->y) &&
	    (FIXED_TO_FLOAT(nodes[bspnum].bbox[0][BOXLEFT  ])-MAXDIST <= p->x) &&
	    (FIXED_TO_FLOAT(nodes[bspnum].bbox[0][BOXRIGHT ])+MAXDIST >= p->x))
		SearchSegInBSP(nodes[bspnum].children[0],p,poly);

	if ((FIXED_TO_FLOAT(nodes[bspnum].bbox[1][BOXBOTTOM])-MAXDIST <= p->y) &&
	    (FIXED_TO_FLOAT(nodes[bspnum].bbox[1][BOXTOP   ])+MAXDIST >= p->y) &&
	    (FIXED_TO_FLOAT(nodes[bspnum].bbox[1][BOXLEFT  ])-MAXDIST <= p->x) &&
	    (FIXED_TO_FLOAT(nodes[bspnum].bbox[1][BOXRIGHT ])+MAXDIST >= p->x))
		SearchSegInBSP(nodes[bspnum].children[1],p,poly);
}

// search for T-intersection problem
// BP : It can be mush more faster doing this at the same time of the splitpoly
// but we must use a different structure : polygone pointing on segs
// segs pointing on polygone and on vertex (too mush complicated, well not
// realy but i am soo lasy), the methode discibed is also better for segs presition
static INT32 SolveTProblem(void)
{
	poly_t *p;
	INT32 i;
	size_t l;

	if (cv_glsolvetjoin.value == 0)
		return 0;

	CONS_Debug(DBG_RENDER, "Solving T-joins. This may take a while. Please wait...\n");
#ifdef HWR_LOADING_SCREEN
	CON_Drawer(); //let the user know what we are doing
	I_FinishUpdate(); // page flip or blit buffer
#endif

	numsplitpoly = 0;

	for (l = 0; l < addsubsector; l++)
	{
		p = extrasubsectors[l].planepoly;
		if (p)
			for (i = 0; i < p->numpts; i++)
				SearchSegInBSP((INT32)numnodes-1, &p->pts[i], p);
	}
	//CONS_Debug(DBG_RENDER, "numsplitpoly %d\n", numsplitpoly);
	return numsplitpoly;
}

#define NEARDIST (0.75f)
#define MYMAX    (10000000000000.0f)

/* Adjust true segs (from the segs lump) to be exactely the same as
 * plane polygone segs
 * This also convert fixed_t point of segs in float (in moste case
 * it share the same vertice
 */
static void AdjustSegs(void)
{
	size_t i, count;
	INT32 j;
	seg_t *lseg;
	poly_t *p;
	INT32 v1found = 0, v2found = 0;
	float nearv1, nearv2;

	for (i = 0; i < numsubsectors; i++)
	{
		count = subsectors[i].numlines;
		lseg = &segs[subsectors[i].firstline];
		p = extrasubsectors[i].planepoly;
		//if (!p)
			//continue;
		for (; count--; lseg++)
		{
			float distv1,distv2,tmp;
			nearv1 = nearv2 = MYMAX;

			// Don't touch polyobject segs. We'll compensate
			// for this when we go about drawing them.
			if (lseg->polyseg)
				continue;

			if (p) {
				for (j = 0; j < p->numpts; j++)
				{
					distv1 = p->pts[j].x - FIXED_TO_FLOAT(lseg->v1->x);
					tmp    = p->pts[j].y - FIXED_TO_FLOAT(lseg->v1->y);
					distv1 = distv1*distv1+tmp*tmp;
					if (distv1 <= nearv1)
					{
						v1found = j;
						nearv1 = distv1;
					}
					// the same with v2
					distv2 = p->pts[j].x - FIXED_TO_FLOAT(lseg->v2->x);
					tmp    = p->pts[j].y - FIXED_TO_FLOAT(lseg->v2->y);
					distv2 = distv2*distv2+tmp*tmp;
					if (distv2 <= nearv2)
					{
						v2found = j;
						nearv2 = distv2;
					}
				}
			}
			if (p && nearv1 <= NEARDIST*NEARDIST)
				// share vertice with segs
				lseg->pv1 = &(p->pts[v1found]);
			else
			{
				// BP: here we can do better, using PointInSeg and compute
				// the right point position also split a polygone side to
				// solve a T-intersection, but too mush work

				// convert fixed vertex to float vertex
				polyvertex_t *pv = HWR_AllocVertex();
				pv->x = FIXED_TO_FLOAT(lseg->v1->x);
				pv->y = FIXED_TO_FLOAT(lseg->v1->y);
				lseg->pv1 = pv;
			}
			if (p && nearv2 <= NEARDIST*NEARDIST)
				lseg->pv2 = &(p->pts[v2found]);
			else
			{
				polyvertex_t *pv = HWR_AllocVertex();
				pv->x = FIXED_TO_FLOAT(lseg->v2->x);
				pv->y = FIXED_TO_FLOAT(lseg->v2->y);
				lseg->pv2 = pv;
			}

			// recompute length
			{
				float x,y;
				x = ((polyvertex_t *)lseg->pv2)->x - ((polyvertex_t *)lseg->pv1)->x
					+ FIXED_TO_FLOAT(FRACUNIT/2);
				y = ((polyvertex_t *)lseg->pv2)->y - ((polyvertex_t *)lseg->pv1)->y
					+ FIXED_TO_FLOAT(FRACUNIT/2);
				lseg->flength = (float)hypot(x, y);
				// BP: debug see this kind of segs
				//if (nearv2 > NEARDIST*NEARDIST || nearv1 > NEARDIST*NEARDIST)
				//    lseg->length = 1;
			}
		}
	}
}


// call this routine after the BSP of a Doom wad file is loaded,
// and it will generate all the convex polys for the hardware renderer
void HWR_CreatePlanePolygons(INT32 bspnum)
{
	poly_t *rootp;
	polyvertex_t *rootpv;
	size_t i;
	fixed_t rootbbox[4];

	CONS_Debug(DBG_RENDER, "Creating polygons, please wait...\n");
#ifdef HWR_LOADING_SCREEN
	ls_count = ls_percent = 0; // reset the loading status
	CON_Drawer(); //let the user know what we are doing
	I_FinishUpdate(); // page flip or blit buffer
#endif

	HWR_ClearPolys();

	// find min/max boundaries of map
	//CONS_Debug(DBG_RENDER, "Looking for boundaries of map...\n");
	M_ClearBox(rootbbox);
	for (i = 0;i < numvertexes; i++)
		M_AddToBox(rootbbox, vertexes[i].x, vertexes[i].y);

	//CONS_Debug(DBG_RENDER, "Generating subsector polygons... %d subsectors\n", numsubsectors);

	HWR_FreeExtraSubsectors();
	// allocate extra data for each subsector present in map
	totsubsectors = numsubsectors + NEWSUBSECTORS;
	extrasubsectors = calloc(totsubsectors, sizeof (*extrasubsectors));
	if (extrasubsectors == NULL)
		I_Error("couldn't malloc extrasubsectors totsubsectors %s\n", sizeu1(totsubsectors));

	// allocate table for back to front drawing of subsectors
	/*gl_drawsubsectors = (INT16 *)malloc(sizeof (*gl_drawsubsectors) * totsubsectors);
	if (!gl_drawsubsectors)
		I_Error("couldn't malloc gl_drawsubsectors\n");*/

	// number of the first new subsector that might be added
	addsubsector = numsubsectors;

	// construct the initial convex poly that encloses the full map
	rootp = HWR_AllocPoly(4);
	rootpv = rootp->pts;

	rootpv->x = FIXED_TO_FLOAT(rootbbox[BOXLEFT  ]);
	rootpv->y = FIXED_TO_FLOAT(rootbbox[BOXBOTTOM]);  //lr
	rootpv++;
	rootpv->x = FIXED_TO_FLOAT(rootbbox[BOXLEFT  ]);
	rootpv->y = FIXED_TO_FLOAT(rootbbox[BOXTOP   ]);  //ur
	rootpv++;
	rootpv->x = FIXED_TO_FLOAT(rootbbox[BOXRIGHT ]);
	rootpv->y = FIXED_TO_FLOAT(rootbbox[BOXTOP   ]);  //ul
	rootpv++;
	rootpv->x = FIXED_TO_FLOAT(rootbbox[BOXRIGHT ]);
	rootpv->y = FIXED_TO_FLOAT(rootbbox[BOXBOTTOM]);  //ll
	rootpv++;

	WalkBSPNode(bspnum, rootp, NULL,rootbbox);

	i = SolveTProblem();
	//CONS_Debug(DBG_RENDER, "%d point divides a polygon line\n",i);
	AdjustSegs();

	//debug debug..
	//if (nobackpoly)
	//    CONS_Debug(DBG_RENDER, "no back polygon %u times\n",nobackpoly);
	//"(should happen only with the deep water trick)"
	//if (skipcut)
	//    CONS_Debug(DBG_RENDER, "%u cuts were skipped because of only one point\n",skipcut);

	//CONS_Debug(DBG_RENDER, "done: %u total subsector convex polygons\n", totalsubsecpolys);
}

#endif //HWRENDER
