#include "doomdef.h"
#include "doomdata.h"
#include "doomtype.h"

#include "nodebuilder.h"

#include "m_bbox.h"
#include "r_defs.h"
#include "r_main.h"
#include "r_state.h"
#include "z_zone.h"

static NodeBuilder *builder = NULL;

static UINT32 HackSeg;	// Seg to force to back of splitter
static UINT32 HackMate;	// Seg to use in front of hack seg

static INT32 SegsStuffed; // I love eating segs.

static const int MaxSegs = 64;
static const int SplitCost = 8;
static const int AAPreference = 16;

static UINT32 VertexMap_Insert(FPrivVert *vert)
{
	UINT32 pos = (UINT32)builder->NumVertices;
	builder->NumVertices++;
	builder->Vertices = Z_Realloc(builder->Vertices, (pos+1) * sizeof(FPrivVert), PU_LEVEL, NULL);
	memcpy(&builder->Vertices[pos], vert, sizeof(FPrivVert));
	return pos;
}

static UINT32 VertexMap_SelectExact(FPrivVert *vert)
{
	FPrivVert *verts = builder->Vertices;
	UINT32 stop = builder->NumVertices;
	UINT32 i;

	for (i = 0; i < stop; ++i)
	{
		if (verts[i].x == vert->x && verts[i].y == vert->y)
			return i;
	}

	// Not present: add it!
	return VertexMap_Insert(vert);
}

// Vertices within this distance of each other will be considered as the same vertex.
#define VERTEX_EPSILON	6		// This is a fixed_t value

static UINT32 VertexMap_SelectClose(FPrivVert *vert)
{
	FPrivVert *verts = builder->Vertices;
	UINT32 stop = builder->NumVertices;
	UINT32 i;

	for (i = 0; i < stop; ++i)
	{
#if VERTEX_EPSILON <= 1
		if (verts[i].x == vert->x && verts[i]->y == y)
#else
		if (abs(verts[i].x - vert->x) < VERTEX_EPSILON &&
			abs(verts[i].y - vert->y) < VERTEX_EPSILON)
#endif
			return i;
	}

	// Not present: add it!
	return VertexMap_Insert(vert);
}

void NodeBuilder_Set(NodeBuilder *nb)
{
	builder = nb;
}

void NodeBuilder_Clear(void)
{
	if (builder == NULL)
		return;

	Z_Free(builder->Nodes);
	Z_Free(builder->Subsectors);
	Z_Free(builder->SubsectorSets);
	Z_Free(builder->Segs);
	Z_Free(builder->Vertices);
	Z_Free(builder->SegList);
	Z_Free(builder->PlaneChecked);
	Z_Free(builder->Planes);

	builder->Nodes = NULL;
	builder->Subsectors = NULL;
	builder->SubsectorSets = NULL;
	builder->Segs = NULL;
	builder->Vertices = NULL;
	builder->SegList = NULL;
	builder->PlaneChecked = NULL;
	builder->Planes = NULL;

	builder->NumNodes = 0;
	builder->NumSubsectors = 0;
	builder->NumSubsectorSets = 0;
	builder->NumSegs = 0;
	builder->NumVertices = 0;
	builder->SegListSize = 0;
	builder->NumPlaneChecked = 0;
	builder->NumPlanes = 0;

	SegsStuffed = 0;
}

static INT32 PushSeg(FPrivSeg *seg)
{
	INT32 pos = builder->NumSegs;
	builder->NumSegs++;
	builder->Segs = Z_Realloc(builder->Segs, (pos+1) * sizeof(FPrivSeg), PU_LEVEL, NULL);
	memcpy(&builder->Segs[pos], seg, sizeof(FPrivSeg));
	return pos;
}

static INT32 PushSegPtr(USegPtr *src)
{
	USegPtr *ptr;
	INT32 pos = builder->SegListSize;

	builder->SegListSize++;
	builder->SegList = Z_Realloc(builder->SegList, (pos+1) * sizeof(USegPtr), PU_LEVEL, NULL);

	ptr = &builder->SegList[pos];
	ptr->SegPtr = src->SegPtr;
	ptr->SegNum = src->SegNum;

	return pos;
}

static INT32 PushNode(node_t *node)
{
	INT32 pos = builder->NumNodes;
	builder->NumNodes++;
	builder->Nodes = Z_Realloc(builder->Nodes, (pos+1) * sizeof(node_t), PU_LEVEL, NULL);
	memcpy(&builder->Nodes[pos], node, sizeof(node_t));
	return pos;
}

static INT32 PushSubsector(subsector_t *node)
{
	INT32 pos = builder->NumSubsectors;
	builder->NumSubsectors++;
	builder->Subsectors = Z_Realloc(builder->Subsectors, (pos+1) * sizeof(subsector_t), PU_LEVEL, NULL);
	memcpy(&builder->Subsectors[pos], node, sizeof(subsector_t));
	return pos;
}

static INT32 PushSubsectorSet(UINT32 set)
{
	INT32 pos = builder->NumSubsectorSets;
	builder->NumSubsectorSets++;
	builder->SubsectorSets = Z_Realloc(builder->SubsectorSets, (pos+1) * sizeof(UINT32), PU_LEVEL, NULL);
	builder->SubsectorSets[pos] = set;
	return pos;
}

static void AddSegToBBox(fixed_t bbox[4], const FPrivSeg *seg)
{
	FPrivVert *v1 = &builder->Vertices[seg->v1];
	FPrivVert *v2 = &builder->Vertices[seg->v2];

	if (v1->x < bbox[BOXLEFT])      bbox[BOXLEFT] = v1->x;
	if (v1->x > bbox[BOXRIGHT])     bbox[BOXRIGHT] = v1->x;
	if (v1->y < bbox[BOXBOTTOM])    bbox[BOXBOTTOM] = v1->y;
	if (v1->y > bbox[BOXTOP])       bbox[BOXTOP] = v1->y;

	if (v2->x < bbox[BOXLEFT])      bbox[BOXLEFT] = v2->x;
	if (v2->x > bbox[BOXRIGHT])     bbox[BOXRIGHT] = v2->x;
	if (v2->y < bbox[BOXBOTTOM])    bbox[BOXBOTTOM] = v2->y;
	if (v2->y > bbox[BOXTOP])       bbox[BOXTOP] = v2->y;
}

static fixed_t CalcSegLength(FPrivSeg *seg)
{
	FPrivVert *v1 = &builder->Vertices[seg->v1];
	FPrivVert *v2 = &builder->Vertices[seg->v2];
	INT64 dx = (v2->x - v1->x)>>1;
	INT64 dy = (v2->y - v1->y)>>1;
	return FixedHypot(dx, dy)<<1;
}

static angle_t CalcSegAngle(FPrivSeg *seg)
{
	FPrivVert *v1 = &builder->Vertices[seg->v1];
	FPrivVert *v2 = &builder->Vertices[seg->v2];
	return R_PointToAngle2(v1->x, v1->y, v2->x, v2->y);
}

static fixed_t CalcSegOffset(FPrivSeg *seg)
{
	FPrivVert *v1 = &builder->Vertices[seg->v1];
	INT64 dx = (seg->side ? seg->linev2x : seg->linev1x) - v1->x;
	INT64 dy = (seg->side ? seg->linev2y : seg->linev1y) - v1->y;
	return FixedHypot(dx>>1, dy>>1)<<1;
}

static void SetNodeFromSeg(node_t *node, const FPrivSeg *pseg)
{
	if (pseg->planenum >= 0)
	{
		FSimpleLine *pline = &builder->Planes[pseg->planenum];
		node->x = pline->x;
		node->y = pline->y;
		node->dx = pline->dx;
		node->dy = pline->dy;
	}
	else
	{
		node->x = builder->Vertices[pseg->v1].x;
		node->y = builder->Vertices[pseg->v1].y;
		node->dx = builder->Vertices[pseg->v2].x - node->x;
		node->dy = builder->Vertices[pseg->v2].y - node->y;
	}
}

static UINT32 SplitSeg(UINT32 segnum, INT32 splitvert, INT32 v1InFront)
{
	FPrivSeg *seg = &builder->Segs[segnum];
	FPrivSeg newseg;

	memcpy(&newseg, seg, sizeof(FPrivSeg));

	if (v1InFront > 0)
	{
		newseg.v1 = splitvert;
		seg->v2 = splitvert;
	}
	else
	{
		seg->v1 = splitvert;
		newseg.v2 = splitvert;
	}

	return PushSeg(&newseg);
}

static double InterceptVector(const node_t *splitter, const FPrivSeg *seg)
{
	double v2x = FixedToDouble(builder->Vertices[seg->v1].x);
	double v2y = FixedToDouble(builder->Vertices[seg->v1].y);
	double v2dx = FixedToDouble(builder->Vertices[seg->v2].x) - v2x;
	double v2dy = FixedToDouble(builder->Vertices[seg->v2].y) - v2y;
	double v1dx = FixedToDouble(splitter->dx);
	double v1dy = FixedToDouble(splitter->dy);

	double den = v1dy*v2dx - v1dx*v2dy;

	double v1x, v1y;
	double num, frac;

	if (fpclassify(den) == FP_ZERO)
		return 0.0;		// parallel

	v1x = FixedToDouble(splitter->x);
	v1y = FixedToDouble(splitter->y);

	num = (v1x - v2x)*v1dy + (v2y - v1y)*v1dx;
	frac = num / den;

	return frac;
}

#define FAR_ENOUGH 17179869184.f		// 4<<32
const double SIDE_EPSILON = 6.5536;

static INT32 ClassifyLine(node_t *node, const FPrivVert *v1, const FPrivVert *v2, int sidev[2])
{
	double d_x1 = FixedToDouble(node->x);
	double d_y1 = FixedToDouble(node->y);
	double d_dx = FixedToDouble(node->dx);
	double d_dy = FixedToDouble(node->dy);
	double d_xv1 = FixedToDouble(v1->x);
	double d_xv2 = FixedToDouble(v2->x);
	double d_yv1 = FixedToDouble(v1->y);
	double d_yv2 = FixedToDouble(v2->y);

	double s_num1 = (d_y1 - d_yv1) * d_dx - (d_x1 - d_xv1) * d_dy;
	double s_num2 = (d_y1 - d_yv2) * d_dx - (d_x1 - d_xv2) * d_dy;

	INT32 nears = 0;

	if (s_num1 <= -FAR_ENOUGH)
	{
		if (s_num2 <= -FAR_ENOUGH)
		{
			sidev[0] = sidev[1] = 1;
			return 1;
		}
		else if (s_num2 >= FAR_ENOUGH)
		{
			sidev[0] = 1;
			sidev[1] = -1;
			return -1;
		}
		nears = 1;
	}
	else if (s_num1 >= FAR_ENOUGH)
	{
		if (s_num2 >= FAR_ENOUGH)
		{
			sidev[0] = sidev[1] = -1;
			return 0;
		}
		else if (s_num2 <= -FAR_ENOUGH)
		{
			sidev[0] = -1;
			sidev[1] = 1;
			return -1;
		}
		nears = 1;
	}
	else
		nears = 2 | (INT32)(fabs(s_num2) < FAR_ENOUGH);

	if (nears)
	{
		double l = 1.0 / (d_dx*d_dx + d_dy*d_dy);

		if (nears & 2)
		{
			double dist = s_num1 * s_num1 * l;
			if (dist < SIDE_EPSILON*SIDE_EPSILON)
				sidev[0] = 0;
			else
				sidev[0] = s_num1 > 0.0 ? -1 : 1;
		}
		else
			sidev[0] = s_num1 > 0.0 ? -1 : 1;

		if (nears & 1)
		{
			double dist = s_num2 * s_num2 * l;
			if (dist < SIDE_EPSILON*SIDE_EPSILON)
				sidev[1] = 0;
			else
				sidev[1] = s_num2 > 0.0 ? -1 : 1;
		}
		else
			sidev[1] = s_num2 > 0.0 ? -1 : 1;
	}
	else
	{
		sidev[0] = s_num1 > 0.0 ? -1 : 1;
		sidev[1] = s_num2 > 0.0 ? -1 : 1;
	}

	if ((sidev[0] | sidev[1]) == 0)
	{
		// seg is coplanar with the splitter, so use its orientation to determine
		// which child it ends up in. If it faces the same direction as the splitter,
		// it goes in front. Otherwise, it goes in back.
		if (node->dx != 0)
		{
			if ((node->dx > 0 && v2->x > v1->x) || (node->dx < 0 && v2->x < v1->x))
				return 0;
			else
				return 1;
		}
		else
		{
			if ((node->dy > 0 && v2->y > v1->y) || (node->dy < 0 && v2->y < v1->y))
				return 0;
			else
				return 1;
		}
	}
	else if (sidev[0] <= 0 && sidev[1] <= 0)
		return 0;
	else if (sidev[0] >= 0 && sidev[1] >= 0)
		return 1;

	return -1;
}

// Given a splitter (node), returns a score based on how "good" the resulting
// split in a set of segs is. Higher scores are better. -1 means this splitter
// splits something it shouldn't and will only be returned if honorNoSplit is
// true. A score of 0 means that the splitter does not split any of the segs
// in the set.
static INT32 Heuristic(node_t *node, UINT32 set, boolean honorNoSplit)
{
	// Set the initial score above 0 so that near vertex anti-weighting is less likely to produce a negative score.
	INT32 score = 1000000;
	INT32 segsInSet = 0;
	INT32 counts[2] = { 0, 0 };
	INT32 realSegs[2] = { 0, 0 };
	INT32 specialSegs[2] = { 0, 0 };
	UINT32 i = set;
	INT32 sidev[2] = {0, 0};
	INT32 side;
	boolean splitter = false;
	double frac;

	while (i != UINT32_MAX)
	{
		const FPrivSeg *test = &builder->Segs[i];

		if (HackSeg == i)
			side = 1;
		else
			side = ClassifyLine(node, &builder->Vertices[test->v1], &builder->Vertices[test->v2], sidev);

		switch (side)
		{
			case 0:	// Seg is on only one side of the partition
				/* FALLTHRU */
			case 1:
				counts[side]++;
				realSegs[side]++;
				if (test->frontsector == test->backsector)
					specialSegs[side]++;
				// Add some weight to the score for unsplit lines
				score += SplitCost;
				break;

			default:	// Seg is cut by the partition
				// Splitters that are too close to a vertex are bad.
				frac = InterceptVector(node, test);
				if (frac < 0.001 || frac > 0.999)
				{
					FPrivVert *v1 = &builder->Vertices[test->v1];
					FPrivVert *v2 = &builder->Vertices[test->v2];
					INT32 penalty;

					double x = FixedToDouble(v1->x), y = FixedToDouble(v1->y);
					x += frac * (FixedToDouble(v2->x) - x);
					y += frac * (FixedToDouble(v2->y) - y);

					if (fabs(x - FixedToDouble(v1->x)) < VERTEX_EPSILON+1 && fabs(y - FixedToDouble(v1->y)) < VERTEX_EPSILON+1)
						return -1;
					else if (fabs(x - FixedToDouble(v2->x)) < VERTEX_EPSILON+1 && fabs(y - FixedToDouble(v2->y)) < VERTEX_EPSILON+1)
						return -1;

					if (frac > 0.999)
						frac = 1.0f - frac;

					penalty = (INT32)(1 / frac);
					score = max(score - penalty, 1);
				}

				counts[0]++;
				counts[1]++;

				realSegs[0]++;
				realSegs[1]++;
				if (test->frontsector == test->backsector)
				{
					specialSegs[0]++;
					specialSegs[1]++;
				}
				break;
		}

		segsInSet++;
		i = test->next;
	}

	// If this line is outside all the others, return a special score
	if (counts[0] == 0 || counts[1] == 0)
		return 0;

	// A splitter must have at least one real seg on each side.
	// Otherwise, a subsector could be left without any way to easily
	// determine which sector it lies inside.
	if (realSegs[0] == 0 || realSegs[1] == 0)
		return -1;

	// Try to avoid splits that leave only "special" segs, so that the generated
	// subsectors have a better chance of choosing the correct sector. This situation
	// is not neccesarily bad, just undesirable.
	if (honorNoSplit && (specialSegs[0] == realSegs[0] || specialSegs[1] == realSegs[1]))
		return -1;

	// Doom maps are primarily axis-aligned lines, so it's usually a good
	// idea to prefer axis-aligned splitters over diagonal ones. Doom originally
	// had special-casing for orthogonal lines, so they performed better. ZDoom
	// does not care about the line's direction, so this is merely a choice to
	// try and improve the final tree.
	if ((node->dx == 0) || (node->dy == 0))
	{
		// If we have to split a seg we would prefer to keep unsplit, give
		// extra precedence to orthogonal lines so that the polyobjects
		// outside the entrance to MAP06 in Hexen MAP02 display properly.
		if (splitter)
			score += segsInSet*8;
		else
			score += segsInSet/AAPreference;
	}

	score += (counts[0] + counts[1]) - abs(counts[0] - counts[1]);

	return score;
}

static void SplitSegs(UINT32 set, node_t *node, UINT32 *outset0, UINT32 *outset1, UINT32 *count0, UINT32 *count1)
{
	UINT32 _count0 = 0;
	UINT32 _count1 = 0;

	(*outset0) = UINT32_MAX;
	(*outset1) = UINT32_MAX;

	while (set != UINT32_MAX)
	{
		FPrivSeg *seg = &builder->Segs[set];
		INT32 next = seg->next;
		INT32 sidev[2], side;

		if (HackSeg == set)
		{
			HackSeg = UINT32_MAX;
			side = 1;
			sidev[0] = sidev[1] = 0;
		}
		else
			side = ClassifyLine(node, &builder->Vertices[seg->v1], &builder->Vertices[seg->v2], sidev);

		switch (side)
		{
			case 0: // seg is entirely in front
				seg->next = (*outset0);
				(*outset0) = set;
				_count0++;
				break;

			case 1: // seg is entirely in back
				seg->next = (*outset1);
				(*outset1) = set;
				_count1++;
				break;

			default: // seg needs to be split
			{
				FPrivVert newvert;
				UINT32 vertnum;
				INT32 seg2;

				double frac = InterceptVector(node, seg);
				newvert.x = builder->Vertices[seg->v1].x;
				newvert.y = builder->Vertices[seg->v1].y;
				newvert.x += DoubleToFixed(frac * FixedToDouble(builder->Vertices[seg->v2].x - newvert.x));
				newvert.y += DoubleToFixed(frac * FixedToDouble(builder->Vertices[seg->v2].y - newvert.y));
				vertnum = VertexMap_SelectClose(&newvert);

				seg2 = SplitSeg(set, vertnum, sidev[0]);

				builder->Segs[seg2].next = (*outset0);
				(*outset0) = seg2;
				builder->Segs[set].next = (*outset1);
				(*outset1) = set;
				_count0++;
				_count1++;

				break;
			}
		}

		set = next;
	}

	*count0 = _count0;
	*count1 = _count1;
}

// Splitters are chosen to coincide with segs in the given set. To reduce the
// number of segs that need to be considered as splitters, segs are grouped into
// according to the planes that they lie on. Because one seg on the plane is just
// as good as any other seg on the plane at defining a split, only one seg from
// each unique plane needs to be considered as a splitter. A result of 0 means
// this set is a convex region. A result of -1 means that there were possible
// splitters, but they all split segs we want to keep intact.
static INT32 SelectSplitter(UINT32 set, node_t *node, INT32 step, boolean nosplit)
{
	INT32 stepleft;
	INT32 bestvalue;
	UINT32 bestseg;
	UINT32 seg;
	boolean nosplitters = false;

	bestvalue = 0;
	bestseg = UINT32_MAX;

	seg = set;
	stepleft = 0;

	memset(builder->PlaneChecked, 0, builder->NumPlaneChecked);

	while (seg != UINT32_MAX)
	{
		FPrivSeg *pseg = &builder->Segs[seg];

		if (--stepleft <= 0)
		{
			INT32 l = pseg->planenum >> 3;
			INT32 r = 1 << (pseg->planenum & 7);

			if (l < 0 || (builder->PlaneChecked[l] & r) == 0)
			{
				INT32 value;

				if (l >= 0)
					builder->PlaneChecked[l] |= r;

				stepleft = step;
				SetNodeFromSeg(node, pseg);

				value = Heuristic(node, set, nosplit);

				if (value > bestvalue)
				{
					bestvalue = value;
					bestseg = seg;
				}
				else if (value < 0)
					nosplitters = true;
			}
		}

		seg = pseg->next;
	}

	// No lines split any others into two sets, so this is a convex region.
	if (bestseg == UINT32_MAX)
		return nosplitters ? -1 : 0;

	SetNodeFromSeg(node, &builder->Segs[bestseg]);

	return 1;
}

// Just create one plane per seg. Should be good enough for mini BSPs.
static void GroupSegPlanesSimple(void)
{
	INT32 segcount = builder->NumSegs, i;

	builder->NumPlanes = segcount;
	builder->Planes = Z_Malloc(segcount * sizeof(FSimpleLine), PU_LEVEL, NULL);

	for (i = 0; i < segcount; ++i)
	{
		FPrivSeg *seg = &builder->Segs[i];
		FSimpleLine *pline = &builder->Planes[i];
		seg->next = i+1;
		seg->planenum = i;
		seg->planefront = true;
		pline->x = builder->Vertices[seg->v1].x;
		pline->y = builder->Vertices[seg->v1].y;
		pline->dx = builder->Vertices[seg->v2].x - builder->Vertices[seg->v1].x;
		pline->dy = builder->Vertices[seg->v2].y - builder->Vertices[seg->v1].y;
	}

	builder->Segs[segcount-1].next = UINT32_MAX;
	builder->NumPlaneChecked = (segcount + 7) / 8;
	builder->PlaneChecked = Z_Calloc(builder->NumPlaneChecked * sizeof(UINT8), PU_LEVEL, NULL);
}

static UINT16 CreateSubsector(UINT32 set, fixed_t bbox[4])
{
	INT32 ssnum, count;

	M_ClearBox(bbox);

	// We cannot actually create the subsector now because the node building
	// process might split a seg in this subsector (because all partner segs
	// must use the same pair of vertices), adding a new seg that hasn't been
	// created yet. After all the nodes are built, then we can create the
	// actual subsectors using the CreateSubsectorsForReal function below.
	ssnum = PushSubsectorSet(set);
	count = 0;

	while (set != UINT32_MAX)
	{
		AddSegToBBox(bbox, &builder->Segs[set]);
		set = builder->Segs[set].next;
		count++;
	}

	SegsStuffed += count;

	return ssnum;
}

static boolean ShoveSegBehind(UINT32 set, node_t *node, UINT32 seg, UINT32 mate)
{
	SetNodeFromSeg(node, &builder->Segs[seg]);

	HackSeg = seg;
	HackMate = mate;

	if (!builder->Segs[seg].planefront)
	{
		node->x += node->dx;
		node->y += node->dy;
		node->dx = -node->dx;
		node->dy = -node->dy;
	}

	return Heuristic(node, set, false) > 0;
}

static boolean CheckSubsector(UINT32 set, node_t *node)
{
	sector_t *sec = NULL;
	UINT32 seg = set;

	do
	{
		if (builder->Segs[seg].frontsector != sec
			// Segs with the same front and back sectors are allowed to reside
			// in a subsector with segs from a different sector, because the
			// only effect they can have on the display is to place masked
			// mid textures in the scene. Since minisegs only mark subsector
			// boundaries, their sector information is unimportant.
			//
			// Update: Lines with the same front and back sectors *can* affect
			// the display if their subsector does not match their front sector.
			/*&& Segs[seg].frontsector != Segs[seg].backsector*/)
		{
			if (sec == NULL)
				sec = builder->Segs[seg].frontsector;
			else
				break;
		}
		seg = builder->Segs[seg].next;
	} while (seg != UINT32_MAX);

	if (seg == UINT32_MAX)
		return false;

	// This is a very simple and cheap "fix" for subsectors with segs
	// from multiple sectors, and it seems ZenNode does something
	// similar. It is the only technique I could find that makes the
	// "transparent water" in nb_bmtrk.wad work properly.
	return ShoveSegBehind(set, node, seg, UINT32_MAX);
}

static UINT16 CreateNode(UINT32 set, UINT32 count, fixed_t bbox[4])
{
	node_t node;
	INT32 selstat;
	INT32 skip = (INT32)(count / MaxSegs);

	// When building GL nodes, count may not be an exact count of the number of segs
	// in the set. That's okay, because we just use it to get a skip count, so an
	// estimate is fine.
	if ((selstat = SelectSplitter(set, &node, skip, true)) > 0 ||
		(skip > 0 && (selstat = SelectSplitter(set, &node, 1, true)) > 0) ||
		(selstat < 0 && (SelectSplitter(set, &node, skip, false) > 0 ||
						(skip > 0 && SelectSplitter(set, &node, 1, false)))) ||
		CheckSubsector(set, &node))
	{
		// Create a normal node
		UINT32 set1, set2;
		UINT32 count1, count2;

		SplitSegs(set, &node, &set1, &set2, &count1, &count2);
		node.children[0] = CreateNode(set1, count1, node.bbox[0]);
		node.children[1] = CreateNode(set2, count2, node.bbox[1]);

		bbox[BOXTOP] = max(node.bbox[0][BOXTOP], node.bbox[1][BOXTOP]);
		bbox[BOXBOTTOM] = min(node.bbox[0][BOXBOTTOM], node.bbox[1][BOXBOTTOM]);
		bbox[BOXLEFT] = min(node.bbox[0][BOXLEFT], node.bbox[1][BOXLEFT]);
		bbox[BOXRIGHT] = max(node.bbox[0][BOXRIGHT], node.bbox[1][BOXRIGHT]);

		return PushNode(&node);
	}
	else
		return NF_SUBSECTOR | CreateSubsector(set, bbox);
}

static int SortSegs(const void *a, const void *b)
{
	const FPrivSeg *x = ((const USegPtr *)a)->SegPtr;
	const FPrivSeg *y = ((const USegPtr *)b)->SegPtr;

	// Segs are grouped into two categories in this order:
	//
	// 1. Segs with different front and back sectors (or no back at all).
	// 2. Segs with the same front and back sectors.
	//
	INT32 xtype, ytype;

	if (x->frontsector == x->backsector)
		xtype = 1;
	else
		xtype = 0;

	if (y->frontsector == y->backsector)
		ytype = 1;
	else
		ytype = 0;

	if (xtype != ytype)
		return xtype - ytype;
	else
		return x->linedef - y->linedef;

	return 0;
}

static void CreateSubsectorsForReal(void)
{
	subsector_t sub;
	UINT32 i, j;

	sub.sector = NULL;
	sub.polynodes = NULL;
	sub.polyList = NULL;
	sub.BSP = NULL;
	sub.numlines = sub.firstline = 0;
	sub.validcount = 0;

	for (i = 0; i < (UINT32)builder->NumSubsectorSets; ++i)
	{
		UINT32 set = builder->SubsectorSets[i];
		UINT32 firstline = (UINT32)builder->SegListSize;

		while (set != UINT32_MAX)
		{
			USegPtr ptr;
			ptr.SegPtr = &builder->Segs[set];
			PushSegPtr(&ptr);
			set = ptr.SegPtr->next;
		}

		sub.numlines = (UINT32)(builder->SegListSize - firstline);
		sub.firstline = (UINT16)firstline;

		// Sort segs by linedef for special effects
		qsort(&builder->SegList[firstline], sub.numlines, sizeof(USegPtr), SortSegs);

		// Convert seg pointers into indices
		for (j = firstline; j < (UINT32)builder->SegListSize; ++j)
			builder->SegList[j].SegNum = (UINT32)(builder->SegList[j].SegPtr - &builder->Segs[0]);

		PushSubsector(&sub);
	}
}

static void BuildTree(void)
{
	fixed_t bbox[4];

	HackSeg = UINT32_MAX;
	HackMate = UINT32_MAX;
	CreateNode(0, builder->NumSegs, bbox);
	CreateSubsectorsForReal();
}

void NodeBuilder_BuildMini(void)
{
	GroupSegPlanesSimple();
	BuildTree();
}

#define TryRealloc(oldsize, newsize, dest, type) \
	size = (size_t)builder->newsize; \
	if (size != bsp->oldsize) { \
		bsp->oldsize = size; \
		bsp->dest = Z_Realloc(bsp->dest, size * sizeof(type), PU_LEVEL, NULL); \
	} \
	size *= sizeof(type);

void NodeBuilder_ExtractMini(minibsp_t *bsp)
{
	UINT32 i;
	size_t size;

	TryRealloc(numverts, NumVertices, verts, vertex_t);

	for (i = 0; i < bsp->numverts; ++i)
	{
		vertex_t *vert = &bsp->verts[i];
		vert->x = builder->Vertices[i].x;
		vert->y = builder->Vertices[i].y;
		vert->floorzset = vert->ceilingzset = 0;
		vert->floorz = vert->ceilingz = 0;
	}

	TryRealloc(numnodes, NumNodes, nodes, node_t);
	memcpy(bsp->nodes, builder->Nodes, size);

	TryRealloc(numsubsectors, NumSubsectors, subsectors, subsector_t);
	memcpy(bsp->subsectors, builder->Subsectors, size);

	TryRealloc(numsegs, NumSegs, segs, seg_t);

	for (i = 0; i < bsp->numsegs; ++i)
	{
		FPrivSeg *org = &builder->Segs[builder->SegList[i].SegNum];
		seg_t *out = &bsp->segs[i];

		out->v1 = &bsp->verts[org->v1];
		out->v2 = &bsp->verts[org->v2];
		out->backsector = org->backsector;
		out->frontsector = org->frontsector;

		out->linedef = lines + org->linedef;
		out->sidedef = sides + org->sidedef;

		out->side = org->side;
		out->length = CalcSegLength(org);
		out->angle = CalcSegAngle(org);
		out->offset = CalcSegOffset(org);

		out->polyseg = org->polynode ? org->polynode->poly : NULL;
		out->polysector = org->polysector;

		out->polybackside = org->backside;
		out->glseg = false;
	}

	bsp->dirty = false;
}

#undef TryRealloc

void NodeBuilder_AddSegs(seg_t *seglist, size_t segcount)
{
	size_t i;

	for (i = 0; i < segcount; ++i)
	{
		FPrivSeg seg;
		FPrivVert vert;

		seg.next = UINT32_MAX;
		seg.planefront = false;
		seg.planenum = UINT32_MAX;

		seg.frontsector = seglist[i].frontsector;
		seg.backsector = seglist[i].backsector;
		seg.polysector = NULL;
		vert.x = seglist[i].v1->x;
		vert.y = seglist[i].v1->y;
		seg.v1 = VertexMap_SelectExact(&vert);
		seg.linev1x = seglist[i].linedef->v1->x;
		seg.linev1y = seglist[i].linedef->v1->y;
		vert.x = seglist[i].v2->x;
		vert.y = seglist[i].v2->y;
		seg.v2 = VertexMap_SelectExact(&vert);
		seg.linev2x = seglist[i].linedef->v1->x;
		seg.linev2y = seglist[i].linedef->v1->y;
		seg.linedef = (INT32)(seglist[i].linedef - lines);
		seg.sidedef = seglist[i].sidedef != NULL ? (INT32)(seglist[i].sidedef - sides) : 0xFFFF;
		seg.polynode = NULL;
		seg.side = seglist[i].side;
		seg.length = seglist[i].length;
		seg.angle = seglist[i].angle;
		seg.offset = seglist[i].offset;

		PushSeg(&seg);
	}
}

void NodeBuilder_AddPolySegs(polynode_t *node)
{
	size_t i;

	for (i = 0; i < node->numsegs; ++i)
	{
		FPrivSeg seg;
		FPrivVert vert;

		polyseg_t *polyseg = &node->segs[i];
		seg_t *wall = polyseg->wall;

		seg.next = UINT32_MAX;
		seg.planefront = false;
		seg.planenum = UINT32_MAX;

		seg.frontsector = wall->polybackside ? wall->frontsector : wall->backsector;
		seg.backsector = node->subsector->sector;
		seg.polysector = node->subsector->sector;
		seg.backside = wall->polybackside;
		vert.x = DoubleToFixed(polyseg->v1.x);
		vert.y = DoubleToFixed(polyseg->v1.y);
		seg.v1 = VertexMap_SelectExact(&vert);
		seg.linev1x = wall->linedef->v1->x;
		seg.linev1y = wall->linedef->v1->y;
		vert.x = DoubleToFixed(polyseg->v2.x);
		vert.y = DoubleToFixed(polyseg->v2.y);
		seg.v2 = VertexMap_SelectExact(&vert);
		seg.linev2x = wall->linedef->v2->x;
		seg.linev2y = wall->linedef->v2->y;
		seg.linedef = (INT32)(wall->linedef - lines);
		seg.sidedef = (INT32)(wall->sidedef - sides);
		seg.polynode = node;
		seg.side = wall->side;
		seg.length = wall->length;
		seg.angle = wall->angle;
		seg.offset = wall->offset;

		PushSeg(&seg);
	}
}
