// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2006      by James Haley
// Copyright (C) 2006-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_polyobj.h
/// \brief Movable segs like in Hexen, but more flexible
///        due to application of dynamic binary space partitioning theory.

#ifndef POLYOBJ_H__
#define POLYOBJ_H__

#include "m_dllist.h"
#include "p_mobj.h"
#include "r_defs.h"

//
// Defines
//

// haleyjd: use zdoom-compatible doomednums

#define POLYOBJ_ANCHOR_DOOMEDNUM     760
#define POLYOBJ_SPAWN_DOOMEDNUM      761

#define POLYOBJ_START_LINE    20

typedef enum
{
	POF_CLIPLINES         = 0x1,       ///< Test against lines for collision
	POF_CLIPPLANES        = 0x2,       ///< Test against tops and bottoms for collision
	POF_SOLID             = 0x3,       ///< Clips things.
	POF_TESTHEIGHT        = 0x4,       ///< Test line collision with heights
	POF_RENDERSIDES       = 0x8,       ///< Renders the sides.
	POF_RENDERTOP         = 0x10,      ///< Renders the top.
	POF_RENDERBOTTOM      = 0x20,      ///< Renders the bottom.
	POF_RENDERPLANES      = 0x30,      ///< Renders top and bottom.
	POF_RENDERALL         = 0x38,      ///< Renders everything.
	POF_INVERT            = 0x40,      ///< Inverts collision (like a cage).
	POF_INVERTPLANES      = 0x80,      ///< Render inside planes.
	POF_INVERTPLANESONLY  = 0x100,     ///< Only render inside planes.
	POF_PUSHABLESTOP      = 0x200,     ///< Pushables will stop movement.
	POF_LDEXEC            = 0x400,     ///< This PO triggers a linedef executor.
	POF_ONESIDE           = 0x800,     ///< Only use the first side of the linedef.
	POF_NOSPECIALS        = 0x1000,    ///< Don't apply sector specials.
	POF_SPLAT             = 0x2000,    ///< Use splat flat renderer (treat cyan pixels as invisible).
} polyobjflags_e;

typedef enum
{
	TMPF_NOINSIDES       = 1,
	TMPF_INTANGIBLE      = 1<<1,
	TMPF_PUSHABLESTOP    = 1<<2,
	TMPF_INVISIBLEPLANES = 1<<3,
	TMPF_EXECUTOR        = 1<<4,
	TMPF_CRUSH           = 1<<5,
	TMPF_SPLAT           = 1<<6,
	//TMPF_DONTCLIPPLANES  = 1<<7,
} textmappolyobjectflags_t;

//
// Polyobject Structure
//

typedef struct polyobj_s
{
	mdllistitem_t link; // for subsector links; must be first

	INT32 id;    // numeric id
	INT32 first; // for hashing: index of first polyobject in this hash chain
	INT32 next;  // for hashing: next polyobject in this hash chain

	INT32 parent; // numeric id of parent polyobject

	size_t segCount;        // number of segs in polyobject
	size_t numSegsAlloc;    // number of segs allocated
	struct seg_s **segs; // the segs, a reallocating array.

	size_t numVertices;            // number of vertices (generally == segCount)
	size_t numVerticesAlloc;       // number of vertices allocated
	vertex_t *origVerts; // original positions relative to spawn spot
	vertex_t *tmpVerts;  // temporary vertex backups for rotation
	vertex_t **vertices; // vertices this polyobject must move

	size_t numLines;          // number of linedefs (generally <= segCount)
	size_t numLinesAlloc;     // number of linedefs allocated
	struct line_s **lines; // linedefs this polyobject must move

	degenmobj_t spawnSpot; // location of spawn spot
	vertex_t    centerPt;  // center point
	fixed_t zdist;         // viewz distance for sorting
	angle_t angle;         // for rotation
	UINT8 attached;         // if true, is attached to a subsector

	fixed_t blockbox[4]; // bounding box for clipping
	UINT8 linked;         // is linked to blockmap
	size_t validcount;   // for clipping: prevents multiple checks
	INT32 damage;        // damage to inflict on stuck things
	fixed_t thrust;      // amount of thrust to put on blocking objects
	INT32 flags;         // Flags for this polyobject

	thinker_t *thinker;  // pointer to a thinker affecting this polyobj

	UINT8 isBad;         // a bad polyobject: should not be rendered/manipulated
	INT32 translucency; // index to translucency tables
	INT16 triggertag;   // Tag of linedef executor to trigger on touch

	struct visplane_s *visplane; // polyobject's visplane, for ease of putting into the list later

	// these are saved for netgames, so do not let Lua touch these!
	INT32 spawnflags; // Flags the polyobject originally spawned with
	INT32 spawntrans; // Translucency the polyobject originally spawned with
} polyobj_t;

//
// Polyobject Blockmap Link Structure
//

typedef struct polymaplink_s
{
	mdllistitem_t link; // for blockmap links
	polyobj_t *po;      // pointer to polyobject
} polymaplink_t;

//
// Polyobject Special Thinkers
//

typedef struct polyrotate_s
{
	thinker_t thinker; // must be first

	INT32 polyObjNum;    // numeric id of polyobject (avoid C pointers here)
	INT32 speed;         // speed of movement per frame
	INT32 distance;      // distance to move
	UINT8 turnobjs;      // turn objects? 0=no, 1=everything but players, 2=everything
} polyrotate_t;

typedef struct polymove_s
{
	thinker_t thinker;  // must be first

	INT32 polyObjNum;   // numeric id of polyobject
	INT32 speed;        // resultant velocity
	fixed_t momx;       // x component of speed along angle
	fixed_t momy;       // y component of speed along angle
	INT32 distance;     // total distance to move
	UINT32 angle;       // angle along which to move
} polymove_t;

// PolyObject waypoint movement return behavior
typedef enum
{
	PWR_STOP,     // Stop after reaching last waypoint
	PWR_WRAP,     // Wrap back to first waypoint
	PWR_COMEBACK, // Repeat sequence in reverse
} polywaypointreturn_e;

typedef struct polywaypoint_s
{
	thinker_t thinker; // must be first

	INT32 polyObjNum;      // numeric id of polyobject
	INT32 speed;           // resultant velocity
	INT32 sequence;        // waypoint sequence #
	INT32 pointnum;        // waypoint #
	INT32 direction;       // 1 for normal, -1 for backwards
	UINT8 returnbehavior;  // behavior after reaching the last waypoint
	UINT8 continuous;      // continuously move - used with PWR_WRAP or PWR_COMEBACK
	UINT8 stophere;        // Will stop after it reaches the next waypoint
} polywaypoint_t;

typedef struct polyslidedoor_s
{
	thinker_t thinker;      // must be first

	INT32 polyObjNum;         // numeric id of affected polyobject
	INT32 delay;              // delay time
	INT32 delayCount;         // delay counter
	INT32 initSpeed;          // initial speed
	INT32 speed;              // speed of motion
	INT32 initDistance;       // initial distance to travel
	INT32 distance;           // current distance to travel
	UINT32 initAngle;         // intial angle
	UINT32 angle;             // angle of motion
	UINT32 revAngle;          // reversed angle to avoid roundoff error
	fixed_t momx;             // x component of speed along angle
	fixed_t momy;             // y component of speed along angle
	UINT8 closing;             // if true, is closing
} polyslidedoor_t;

typedef struct polyswingdoor_s
{
	thinker_t thinker; // must be first

	INT32 polyObjNum;    // numeric id of affected polyobject
	INT32 delay;         // delay time
	INT32 delayCount;    // delay counter
	INT32 initSpeed;     // initial speed
	INT32 speed;         // speed of rotation
	INT32 initDistance;  // initial distance to travel
	INT32 distance;      // current distance to travel
	UINT8 closing;        // if true, is closing
} polyswingdoor_t;

typedef struct polydisplace_s
{
	thinker_t thinker; // must be first

	INT32 polyObjNum;
	struct sector_s *controlSector;
	fixed_t dx;
	fixed_t dy;
	fixed_t oldHeights;
} polydisplace_t;

typedef struct polyrotdisplace_s
{
	thinker_t thinker; // must be first

	INT32 polyObjNum;
	struct sector_s *controlSector;
	fixed_t rotscale;
	UINT8 turnobjs;
	fixed_t oldHeights;
} polyrotdisplace_t;

typedef struct polyfade_s
{
	thinker_t thinker; // must be first

	INT32 polyObjNum;
	INT32 sourcevalue;
	INT32 destvalue;
	boolean docollision;
	boolean doghostfade;
	boolean ticbased;
	INT32 duration;
	INT32 timer;
} polyfade_t;

//
// Line Activation Data Structures
//

typedef struct polyrotdata_s
{
	INT32 polyObjNum;   // numeric id of polyobject to affect
	INT32 direction;    // direction of rotation
	INT32 speed;        // angular speed
	INT32 distance;     // distance to move
	UINT8 turnobjs;     // rotate objects being carried?
	UINT8 overRide;     // if true, will override any action on the object
} polyrotdata_t;

typedef struct polymovedata_s
{
	INT32 polyObjNum;   // numeric id of polyobject to affect
	fixed_t distance;   // distance to move
	fixed_t speed;      // linear speed
	angle_t angle;      // angle of movement
	UINT8 overRide;     // if true, will override any action on the object
} polymovedata_t;

typedef enum
{
	PWF_REVERSE = 1,    // Move through waypoints in reverse order
	PWF_LOOP    = 1<<1, // Loop movement (used with PWR_WRAP or PWR_COMEBACK)
} polywaypointflags_e;

typedef struct polywaypointdata_s
{
	INT32 polyObjNum;     // numeric id of polyobject to affect
	INT32 sequence;       // waypoint sequence #
	fixed_t speed;        // linear speed
	UINT8 returnbehavior; // behavior after reaching the last waypoint
	UINT8 flags;          // PWF_ flags
} polywaypointdata_t;

// polyobject door types
typedef enum
{
	POLY_DOOR_SLIDE,
	POLY_DOOR_SWING,
} polydoor_e;

typedef struct polydoordata_s
{
	INT32 polyObjNum;     // numeric id of polyobject to affect
	INT32 doorType;       // polyobj door type
	INT32 speed;          // linear or angular speed
	angle_t angle;        // for slide door only, angle of motion
	INT32 distance;       // distance to move
	INT32 delay;          // delay time after opening
} polydoordata_t;

typedef struct polydisplacedata_s
{
	INT32 polyObjNum;
	struct sector_s *controlSector;
	fixed_t dx;
	fixed_t dy;
} polydisplacedata_t;

typedef struct polyrotdisplacedata_s
{
	INT32 polyObjNum;
	struct sector_s *controlSector;
	fixed_t rotscale;
	UINT8 turnobjs;
} polyrotdisplacedata_t;

typedef struct polyflagdata_s
{
	INT32 polyObjNum;
	INT32 speed;
	UINT32 angle;
	fixed_t momx;
} polyflagdata_t;

typedef struct polyfadedata_s
{
	INT32 polyObjNum;
	INT32 destvalue;
	boolean docollision;
	boolean doghostfade;
	boolean ticbased;
	INT32 speed;
} polyfadedata_t;

//
// Functions
//

polyobj_t *Polyobj_GetForNum(INT32 id);
void Polyobj_InitLevel(void);
void Polyobj_MoveOnLoad(polyobj_t *po, angle_t angle, fixed_t x, fixed_t y);
boolean P_PointInsidePolyobj(polyobj_t *po, fixed_t x, fixed_t y);
boolean P_MobjTouchingPolyobj(polyobj_t *po, mobj_t *mo);
boolean P_MobjInsidePolyobj(polyobj_t *po, mobj_t *mo);
boolean P_BBoxInsidePolyobj(polyobj_t *po, fixed_t *bbox);

// thinkers (needed in p_saveg.c)
void T_PolyObjRotate(polyrotate_t *);
void T_PolyObjMove  (polymove_t *);
void T_PolyObjWaypoint (polywaypoint_t *);
void T_PolyDoorSlide(polyslidedoor_t *);
void T_PolyDoorSwing(polyswingdoor_t *);
void T_PolyObjDisplace  (polydisplace_t *);
void T_PolyObjRotDisplace  (polyrotdisplace_t *);
void T_PolyObjFlag  (polymove_t *);
void T_PolyObjFade  (polyfade_t *);

boolean EV_DoPolyDoor(polydoordata_t *);
boolean EV_DoPolyObjMove(polymovedata_t *);
boolean EV_DoPolyObjWaypoint(polywaypointdata_t *);
boolean EV_DoPolyObjRotate(polyrotdata_t *);
boolean EV_DoPolyObjDisplace(polydisplacedata_t *);
boolean EV_DoPolyObjRotDisplace(polyrotdisplacedata_t *);
boolean EV_DoPolyObjFlag(polyflagdata_t *);
boolean EV_DoPolyObjFade(polyfadedata_t *);


//
// External Variables
//

extern polyobj_t *PolyObjects;
extern INT32 numPolyObjects;
extern polymaplink_t **polyblocklinks; // polyobject blockmap

#endif

// EOF
