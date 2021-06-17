#ifndef __NODEBUILDER_H__
#define __NODEBUILDER_H__

#include "doomdef.h"
#include "doomdata.h"
#include "doomtype.h"

#include "r_defs.h"

typedef struct FPrivSeg_s
{
	INT32 v1, v2;
	fixed_t linev1x, linev1y;
	fixed_t linev2x, linev2y;
	INT32 sidedef;
	INT32 linedef;
	INT32 side;
	fixed_t length;
	fixed_t offset;
	angle_t angle;
	sector_t *frontsector;
	sector_t *backsector;
	polynode_t *polynode;
	sector_t *polysector;
	boolean backside;
	UINT32 next;

	INT32 planenum;
	boolean planefront;
} FPrivSeg;

typedef struct FPrivVert_s
{
	fixed_t x, y;
} FPrivVert;

typedef struct FSimpleLine_s
{
	fixed_t x, y, dx, dy;
} FSimpleLine;

typedef union USegPtr_s
{
	UINT32 SegNum;
	FPrivSeg *SegPtr;
} USegPtr;

typedef struct
{
	node_t *Nodes;
	subsector_t *Subsectors;
	UINT32 *SubsectorSets;
	FPrivSeg *Segs;
	FPrivVert *Vertices;
	USegPtr *SegList;
	UINT8 *PlaneChecked;
	FSimpleLine *Planes;

	INT32 NumNodes;
	INT32 NumSubsectors;
	INT32 NumSubsectorSets;
	INT32 NumSegs;
	INT32 NumVertices;
	INT32 SegListSize;
	INT32 NumPlaneChecked;
	INT32 NumPlanes;
} NodeBuilder;

void NodeBuilder_Set(NodeBuilder *nb);
void NodeBuilder_Clear(void);
void NodeBuilder_AddSegs(seg_t *seglist, size_t segcount);
void NodeBuilder_AddPolySegs(polynode_t *node);
void NodeBuilder_BuildMini(void);
void NodeBuilder_ExtractMini(minibsp_t *bsp);

#endif // __NODEBUILDER_H__
