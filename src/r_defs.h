// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_defs.h
/// \brief Refresh/rendering module, shared data struct definitions

#ifndef __R_DEFS__
#define __R_DEFS__

// Some more or less basic data types we depend on.
#include "m_fixed.h"
#include "m_vector.h"

// We rely on the thinker data struct to handle sound origins in sectors.
#include "d_think.h"
// SECTORS do store MObjs anyway.
#include "p_mobj.h"

#include "screen.h" // MAXVIDWIDTH, MAXVIDHEIGHT

#ifdef HWRENDER
#include "m_aatree.h"
#endif

#include "taglist.h"

//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//
typedef struct
{
	INT32 first;
	INT32 last;
} cliprange_t;

// Silhouette, needed for clipping segs (mainly) and sprites representing things.
#define SIL_NONE   0
#define SIL_BOTTOM 1
#define SIL_TOP    2
#define SIL_BOTH   3

// This could be wider for >8 bit display.
// Indeed, true color support is possible precalculating 24bpp lightmap/colormap LUT
// from darkening PLAYPAL to all black.
// Could even use more than 32 levels.
typedef UINT8 lighttable_t;

#define NUM_PALETTE_ENTRIES 256
#define DEFAULT_STARTTRANSCOLOR 96

#define CMF_FADEFULLBRIGHTSPRITES  1
#define CMF_FOG 4

// ExtraColormap type. Use for extra_colormaps from now on.
typedef struct extracolormap_s
{
	UINT8 fadestart, fadeend;
	UINT8 flags;

	// store rgba values in combined bitwise
	// also used in OpenGL instead lighttables
	INT32 rgba; // similar to maskcolor in sw mode
	INT32 fadergba; // The colour the colourmaps fade to

	lighttable_t *colormap;

#ifdef HWRENDER
	// The id of the hardware lighttable. Zero means it does not exist yet.
	UINT32 gl_lighttable_id;
#endif

#ifdef EXTRACOLORMAPLUMPS
	lumpnum_t lump; // for colormap lump matching, init to LUMPERROR
	char lumpname[9]; // for netsyncing
#endif

	struct extracolormap_s *next;
	struct extracolormap_s *prev;
} extracolormap_t;

//
// INTERNAL MAP TYPES used by play and refresh
//

/** Your plain vanilla vertex.
  */
typedef struct
{
	fixed_t x, y;
	boolean floorzset, ceilingzset;
	fixed_t floorz, ceilingz;
} vertex_t;

// Forward of linedefs, for sectors.
struct line_s;

/** Degenerate version of ::mobj_t, storing only a location.
  * Used for sound origins in sectors, hoop centers, and the like. Does not
  * handle sound from moving objects (doppler), because position is probably
  * just buffered, not updated.
  */
typedef struct
{
	thinker_t thinker; ///< Not used for anything.
	fixed_t x;         ///< X coordinate.
	fixed_t y;         ///< Y coordinate.
	fixed_t z;         ///< Z coordinate.
} degenmobj_t;

#include "p_polyobj.h"

// Store fake planes in a resizable array insted of just by
// heightsec. Allows for multiple fake planes.
/** Flags describing 3Dfloor behavior and appearance.
  */
typedef enum
{
	FOF_EXISTS            = 0x1,        ///< Always set, to check for validity.
	FOF_BLOCKPLAYER       = 0x2,        ///< Solid to player, but nothing else
	FOF_BLOCKOTHERS       = 0x4,        ///< Solid to everything but player
	FOF_SOLID             = 0x6,        ///< Clips things.
	FOF_RENDERSIDES       = 0x8,        ///< Renders the sides.
	FOF_RENDERPLANES      = 0x10,       ///< Renders the floor/ceiling.
	FOF_RENDERALL         = 0x18,       ///< Renders everything.
	FOF_SWIMMABLE         = 0x20,       ///< Is a water block.
	FOF_NOSHADE           = 0x40,       ///< Messes with the lighting?
	FOF_CUTSOLIDS         = 0x80,       ///< Cuts out hidden solid pixels.
	FOF_CUTEXTRA          = 0x100,      ///< Cuts out hidden translucent pixels.
	FOF_CUTLEVEL          = 0x180,      ///< Cuts out all hidden pixels.
	FOF_CUTSPRITES        = 0x200,      ///< Final step in making 3D water.
	FOF_BOTHPLANES        = 0x400,      ///< Render inside and outside planes.
	FOF_EXTRA             = 0x800,      ///< Gets cut by ::FOF_CUTEXTRA.
	FOF_TRANSLUCENT       = 0x1000,     ///< See through!
	FOF_FOG               = 0x2000,     ///< Fog "brush."
	FOF_INVERTPLANES      = 0x4000,     ///< Only render inside planes.
	FOF_ALLSIDES          = 0x8000,     ///< Render inside and outside sides.
	FOF_INVERTSIDES       = 0x10000,    ///< Only render inside sides.
	FOF_DOUBLESHADOW      = 0x20000,    ///< Make two lightlist entries to reset light?
	FOF_FLOATBOB          = 0x40000,    ///< Floats on water and bobs if you step on it.
	FOF_NORETURN          = 0x80000,    ///< Used with ::FOF_CRUMBLE. Will not return to its original position after falling.
	FOF_CRUMBLE           = 0x100000,   ///< Falls 2 seconds after being stepped on, and randomly brings all touching crumbling 3dfloors down with it, providing their master sectors share the same tag (allows crumble platforms above or below, to also exist).
	FOF_GOOWATER          = 0x200000,   ///< Used with ::FOF_SWIMMABLE. Makes thick bouncey goop.
	FOF_MARIO             = 0x400000,   ///< Acts like a question block when hit from underneath. Goodie spawned at top is determined by master sector.
	FOF_BUSTUP            = 0x800000,   ///< You can spin through/punch this block and it will crumble!
	FOF_QUICKSAND         = 0x1000000,  ///< Quicksand!
	FOF_PLATFORM          = 0x2000000,  ///< You can jump up through this to the top.
	FOF_REVERSEPLATFORM   = 0x4000000,  ///< A fall-through floor in normal gravity, a platform in reverse gravity.
	FOF_INTANGIBLEFLATS   = 0x6000000,  ///< Both flats are intangible, but the sides are still solid.
	FOF_RIPPLE            = 0x8000000,  ///< Ripple the flats
	FOF_COLORMAPONLY      = 0x10000000, ///< Only copy the colormap, not the lightlevel
	FOF_BOUNCY            = 0x20000000, ///< Bounces players
	FOF_SPLAT             = 0x40000000, ///< Use splat flat renderer (treat cyan pixels as invisible)
} ffloortype_e;

typedef enum
{
	FF_OLD_EXISTS            = 0x1,
	FF_OLD_BLOCKPLAYER       = 0x2,
	FF_OLD_BLOCKOTHERS       = 0x4,
	FF_OLD_SOLID             = 0x6,
	FF_OLD_RENDERSIDES       = 0x8,
	FF_OLD_RENDERPLANES      = 0x10,
	FF_OLD_RENDERALL         = 0x18,
	FF_OLD_SWIMMABLE         = 0x20,
	FF_OLD_NOSHADE           = 0x40,
	FF_OLD_CUTSOLIDS         = 0x80,
	FF_OLD_CUTEXTRA          = 0x100,
	FF_OLD_CUTLEVEL          = 0x180,
	FF_OLD_CUTSPRITES        = 0x200,
	FF_OLD_BOTHPLANES        = 0x400,
	FF_OLD_EXTRA             = 0x800,
	FF_OLD_TRANSLUCENT       = 0x1000,
	FF_OLD_FOG               = 0x2000,
	FF_OLD_INVERTPLANES      = 0x4000,
	FF_OLD_ALLSIDES          = 0x8000,
	FF_OLD_INVERTSIDES       = 0x10000,
	FF_OLD_DOUBLESHADOW      = 0x20000,
	FF_OLD_FLOATBOB          = 0x40000,
	FF_OLD_NORETURN          = 0x80000,
	FF_OLD_CRUMBLE           = 0x100000,
	FF_OLD_SHATTERBOTTOM     = 0x200000,
	FF_OLD_GOOWATER          = 0x200000,
	FF_OLD_MARIO             = 0x400000,
	FF_OLD_BUSTUP            = 0x800000,
	FF_OLD_QUICKSAND         = 0x1000000,
	FF_OLD_PLATFORM          = 0x2000000,
	FF_OLD_REVERSEPLATFORM   = 0x4000000,
	FF_OLD_INTANGIBLEFLATS   = 0x6000000,
	FF_OLD_SHATTER           = 0x8000000,
	FF_OLD_SPINBUST          = 0x10000000,
	FF_OLD_STRONGBUST        = 0x20000000,
	FF_OLD_RIPPLE            = 0x40000000,
	FF_OLD_COLORMAPONLY      = 0x80000000,
} oldffloortype_e;

typedef enum
{
	FB_PUSHABLES   = 0x1, // Bustable by pushables
	FB_EXECUTOR    = 0x2, // Trigger linedef executor
	FB_ONLYBOTTOM  = 0x4, // Only bustable from below
} ffloorbustflags_e;

typedef enum
{
	BT_TOUCH,
	BT_SPINBUST,
	BT_REGULAR,
	BT_STRONG,
} busttype_e;

typedef enum
{
	SECPORTAL_LINE,    // Works similar to a line portal
	SECPORTAL_SKYBOX,  // Uses the skybox object as the reference view
	SECPORTAL_PLANE,   // Eternity Engine's plane portal type
	SECPORTAL_HORIZON, // Eternity Engine's horizon portal type
	SECPORTAL_OBJECT,  // Uses an object as the reference view
	SECPORTAL_FLOOR,   // Uses a sector as the reference view; the view height is aligned with the sector's floor
	SECPORTAL_CEILING, // Uses a sector as the reference view; the view height is aligned with the sector's ceiling
	SECPORTAL_NONE = 0xFF
} secportaltype_e;

typedef struct sectorportal_s
{
	secportaltype_e type;
	union {
		struct {
			struct line_s *start;
			struct line_s *dest;
		} line;
		struct sector_s *sector;
		struct mobj_s *mobj;
	};
	struct {
		fixed_t x, y;
	} origin;
} sectorportal_t;

typedef struct ffloor_s
{
	fixed_t *topheight;
	INT32 *toppic;
	INT16 *toplightlevel;
	fixed_t *topxoffs;
	fixed_t *topyoffs;
	fixed_t *topxscale;
	fixed_t *topyscale;
	angle_t *topangle;

	fixed_t *bottomheight;
	INT32 *bottompic;
	fixed_t *bottomxoffs;
	fixed_t *bottomyoffs;
	fixed_t *bottomxscale;
	fixed_t *bottomyscale;
	angle_t *bottomangle;

	// Pointers to pointers. Yup.
	struct pslope_s **t_slope;
	struct pslope_s **b_slope;

	size_t secnum;
	ffloortype_e fofflags;
	struct line_s *master;

	struct sector_s *target;

	struct ffloor_s *next;
	struct ffloor_s *prev;

	INT32 lastlight;
	INT32 alpha;
	UINT8 blend; // blendmode
	tic_t norender; // for culling

	// Only relevant for FOF_BUSTUP
	ffloorbustflags_e bustflags;
	UINT8 busttype;
	INT16 busttag;

	// Only relevant for FOF_QUICKSAND
	fixed_t sinkspeed;
	fixed_t friction;

	// Only relevant for FOF_BOUNCY
	fixed_t bouncestrength;

	// these are saved for netgames, so do not let Lua touch these!
	ffloortype_e spawnflags; // flags the 3D floor spawned with
	INT32 spawnalpha; // alpha the 3D floor spawned with

	void *fadingdata; // fading FOF thinker
} ffloor_t;


// This struct holds information for shadows casted by 3D floors.
// This information is contained inside the sector_t and is used as the base
// information for casted shadows.
typedef struct lightlist_s
{
	fixed_t height;
	INT16 *lightlevel;
	extracolormap_t **extra_colormap; // pointer-to-a-pointer, so we can react to colormap changes
	INT32 flags;
	ffloor_t *caster;
	struct pslope_s *slope; // FOF_DOUBLESHADOW makes me have to store this pointer here. Bluh bluh.
} lightlist_t;


// This struct is used for rendering walls with shadows casted on them...
typedef struct r_lightlist_s
{
	fixed_t height;
	fixed_t heightstep;
	fixed_t botheight;
	fixed_t botheightstep;
	fixed_t startheight; // for repeating midtextures
	INT16 lightlevel;
	extracolormap_t *extra_colormap;
	lighttable_t *rcolormap;
	ffloortype_e flags;
	INT32 lightnum;
} r_lightlist_t;

// Slopes
typedef enum {
	SL_NOPHYSICS = 1, /// This plane will have no physics applied besides the positioning.
	SL_DYNAMIC = 1<<1, /// This plane slope will be assigned a thinker to make it dynamic.
} slopeflags_t;

typedef struct pslope_s
{
	UINT16 id; // The number of the slope, mostly used for netgame syncing purposes
	struct pslope_s *next; // Make a linked list of dynamic slopes, for easy reference later

	// The plane's definition.
	vector3_t o;		/// Plane origin.
	vector3_t normal;	/// Plane normal.

	vector2_t d;		/// Precomputed normalized projection of the normal over XY.
	fixed_t zdelta;		/// Precomputed Z unit increase per XY unit.

	// This values only check and must be updated if the slope itself is modified
	angle_t zangle;		/// Precomputed angle of the plane going up from the ground (not measured in degrees).
	angle_t xydirection;/// Precomputed angle of the normal's projection on the XY plane.

	dvector3_t dorigin;
	dvector3_t dnormdir;

	double dzdelta;

	boolean moved : 1;

	UINT8 flags; // Slope options
} pslope_t;

typedef enum
{
	// flipspecial - planes with effect
	MSF_FLIPSPECIAL_FLOOR       =  1,
	MSF_FLIPSPECIAL_CEILING     =  1<<1,
	MSF_FLIPSPECIAL_BOTH        =  (MSF_FLIPSPECIAL_FLOOR|MSF_FLIPSPECIAL_CEILING),
	// triggerspecial - conditions under which plane touch causes effect
	MSF_TRIGGERSPECIAL_TOUCH    =  1<<2,
	MSF_TRIGGERSPECIAL_HEADBUMP =  1<<3,
	// triggerline - conditions for linedef executor triggering
	MSF_TRIGGERLINE_PLANE       =  1<<4, // require plane touch
	MSF_TRIGGERLINE_MOBJ        =  1<<5, // allow non-pushable mobjs to trigger
	// invertprecip - inverts presence of precipitation
	MSF_INVERTPRECIP            =  1<<6,
	MSF_GRAVITYFLIP             =  1<<7,
	MSF_HEATWAVE                =  1<<8,
	MSF_NOCLIPCAMERA            =  1<<9,
} sectorflags_t;

typedef enum
{
	SSF_OUTERSPACE = 1,
	SSF_DOUBLESTEPUP = 1<<1,
	SSF_NOSTEPDOWN = 1<<2,
	SSF_WINDCURRENT = 1<<3,
	SSF_CONVEYOR = 1<<4,
	SSF_SPEEDPAD = 1<<5,
	SSF_STARPOSTACTIVATOR = 1<<6,
	SSF_EXIT = 1<<7,
	SSF_SPECIALSTAGEPIT = 1<<8,
	SSF_RETURNFLAG = 1<<9,
	SSF_REDTEAMBASE = 1<<10,
	SSF_BLUETEAMBASE = 1<<11,
	SSF_FAN = 1<<12,
	SSF_SUPERTRANSFORM = 1<<13,
	SSF_FORCESPIN = 1<<14,
	SSF_ZOOMTUBESTART = 1<<15,
	SSF_ZOOMTUBEEND = 1<<16,
	SSF_FINISHLINE = 1<<17,
	SSF_ROPEHANG = 1<<18,
	SSF_JUMPFLIP = 1<<19,
	SSF_GRAVITYOVERRIDE = 1<<20, // combine with MSF_GRAVITYFLIP
} sectorspecialflags_t;

typedef enum
{
	SD_NONE = 0,
	SD_GENERIC = 1,
	SD_WATER = 2,
	SD_FIRE = 3,
	SD_LAVA = 4,
	SD_ELECTRIC = 5,
	SD_SPIKE = 6,
	SD_DEATHPITTILT = 7,
	SD_DEATHPITNOTILT = 8,
	SD_INSTAKILL = 9,
	SD_SPECIALSTAGE = 10,
} sectordamage_t;

typedef enum
{
	TO_PLAYER = 0,
	TO_ALLPLAYERS = 1,
	TO_MOBJ = 2,
	TO_PLAYEREMERALDS = 3, // only for binary backwards compatibility: check player emeralds
	TO_PLAYERNIGHTS = 4, // only for binary backwards compatibility: check NiGHTS mare
} triggerobject_t;

typedef enum
{
	CRUMBLE_NONE, // No crumble thinker
	CRUMBLE_WAIT, // Don't float on water because this is supposed to wait for a crumble
	CRUMBLE_ACTIVATED, // Crumble thinker activated, but hasn't fallen yet
	CRUMBLE_FALL, // Crumble thinker is falling
	CRUMBLE_RESTORE, // Crumble thinker is about to restore to original position
} crumblestate_t;

//
// The SECTORS record, at runtime.
// Stores things/mobjs.
//
typedef struct sector_s
{
	fixed_t floorheight;
	fixed_t ceilingheight;
	INT32 floorpic;
	INT32 ceilingpic;
	INT16 lightlevel;
	INT16 special;
	taglist_t tags;

	// origin for any sounds played by the sector
	// also considered the center for e.g. Mario blocks
	degenmobj_t soundorg;

	// if == validcount, already checked
	size_t validcount;

	// list of mobjs in sector
	mobj_t *thinglist;

	// thinker_ts for reversable actions
	void *floordata; // floor move thinker
	void *ceilingdata; // ceiling move thinker
	void *lightingdata; // lighting change thinker
	void *fadecolormapdata; // fade colormap thinker

	// floor and ceiling texture offsets
	fixed_t floorxoffset, flooryoffset;
	fixed_t ceilingxoffset, ceilingyoffset;

	// floor and ceiling texture scale
	fixed_t floorxscale, flooryscale;
	fixed_t ceilingxscale, ceilingyscale;

	// flat angle
	angle_t floorangle;
	angle_t ceilingangle;

	INT32 heightsec; // other sector, or -1 if no other sector
	INT32 camsec; // used for camera clipping

	// floor and ceiling lighting
	INT16 floorlightlevel, ceilinglightlevel;
	boolean floorlightabsolute, ceilinglightabsolute; // absolute or relative to sector's light level?
	INT32 floorlightsec, ceilinglightsec; // take floor/ceiling light level from another sector

	INT32 crumblestate; // used for crumbling and bobbing

	// list of mobjs that are at least partially in the sector
	// thinglist is a subset of touching_thinglist
	struct msecnode_s *touching_thinglist;

	size_t linecount;
	struct line_s **lines; // [linecount] size

	// Improved fake floor hack
	ffloor_t *ffloors;
	size_t *attached;
	boolean *attachedsolid;
	size_t numattached;
	size_t maxattached;
	lightlist_t *lightlist;
	INT32 numlights;
	boolean moved;

	// per-sector colormaps!
	extracolormap_t *extra_colormap;
	boolean colormap_protected;

	fixed_t gravity; // per-sector gravity factor
	fixed_t *gravityptr; // For binary format: Read gravity from floor height of master sector

	sectorflags_t flags;
	sectorspecialflags_t specialflags;
	UINT8 damagetype;

	// Linedef executor triggering
	mtag_t triggertag; // tag to call upon triggering
	UINT8 triggerer; // who can trigger?

	fixed_t friction;

	// Sprite culling feature
	struct line_s *cullheight;

	// Current speed of ceiling/floor. For Knuckles to hold onto stuff.
	fixed_t floorspeed, ceilspeed;

	// list of precipitation mobjs in sector
	precipmobj_t *preciplist;
	struct mprecipsecnode_s *touching_preciplist;

	// Eternity engine slope
	pslope_t *f_slope; // floor slope
	pslope_t *c_slope; // ceiling slope
	boolean hasslope; // The sector, or one of its visible FOFs, contains a slope

	// for fade thinker
	INT16 spawn_lightlevel;

	// colormap structure
	extracolormap_t *spawn_extra_colormap;

	// portals
	UINT32 portal_floor;
	UINT32 portal_ceiling;
} sector_t;

//
// Move clipping aid for linedefs.
//
typedef enum
{
	ST_HORIZONTAL,
	ST_VERTICAL,
	ST_POSITIVE,
	ST_NEGATIVE
} slopetype_t;

#define SPECIAL_HORIZON_LINE 41

#define SPECIAL_SECTOR_SETPORTAL 6

#define NUMLINEARGS 10
#define NUMLINESTRINGARGS 2

#define NO_SIDEDEF 0xFFFFFFFF

typedef struct line_s
{
	// Vertices, from v1 to v2.
	vertex_t *v1;
	vertex_t *v2;

	fixed_t dx, dy; // Precalculated v2 - v1 for side checking.
	angle_t angle; // Precalculated angle between dx and dy

	// Animation related.
	INT16 flags;
	INT16 special;
	taglist_t tags;
	INT32 args[NUMLINEARGS];
	char *stringargs[NUMLINESTRINGARGS];

	// Visual appearance: sidedefs.
	UINT32 sidenum[2]; // sidenum[1] will be NO_SIDEDEF if one-sided
	fixed_t alpha; // translucency
	UINT8 blendmode; // blendmode
	INT32 executordelay;

	fixed_t bbox[4]; // bounding box for the extent of the linedef

	// To aid move clipping.
	slopetype_t slopetype;

	// Front and back sector.
	// Note: redundant? Can be retrieved from SideDefs.
	sector_t *frontsector;
	sector_t *backsector;

	size_t validcount; // if == validcount, already checked
	polyobj_t *polyobj; // Belongs to a polyobject?

	INT16 callcount; // no. of calls left before triggering, for the "X calls" linedef specials, defaults to 0

	UINT32 secportal; // transferred sector portal
} line_t;

typedef struct
{
	// add this to the calculated texture column
	fixed_t textureoffset;

	// add this to the calculated texture top
	fixed_t rowoffset;

	// per-texture offsets for UDMF
	fixed_t offsetx_top, offsetx_mid, offsetx_bottom;
	fixed_t offsety_top, offsety_mid, offsety_bottom;

	fixed_t scalex_top, scalex_mid, scalex_bottom;
	fixed_t scaley_top, scaley_mid, scaley_bottom;

	// Texture indices.
	// We do not maintain names here.
	INT32 toptexture, bottomtexture, midtexture;

	// Linedef the sidedef belongs to
	line_t *line;

	// Sector the sidedef is facing.
	sector_t *sector;

	INT16 special; // the special of the linedef this side belongs to
	INT16 repeatcnt; // # of times to repeat midtexture

	extracolormap_t *colormap_data; // storage for colormaps; not applied to sectors.
} side_t;

//
// A subsector.
// References a sector.
// Basically, this is a list of linesegs, indicating the visible walls that define
//  (all or some) sides of a convex BSP leaf.
//
typedef struct subsector_s
{
	sector_t *sector;
	INT16 numlines;
	UINT32 firstline;
	struct polyobj_s *polyList; // haleyjd 02/19/06: list of polyobjects
	size_t validcount;
} subsector_t;

// Sector list node showing all sectors an object appears in.
//
// There are two threads that flow through these nodes. The first thread
// starts at touching_thinglist in a sector_t and flows through the m_thinglist_next
// links to find all mobjs that are entirely or partially in the sector.
// The second thread starts at touching_sectorlist in an mobj_t and flows
// through the m_sectorlist_next links to find all sectors a thing touches. This is
// useful when applying friction or push effects to sectors. These effects
// can be done as thinkers that act upon all objects touching their sectors.
// As an mobj moves through the world, these nodes are created and
// destroyed, with the links changed appropriately.
//
// For the links, NULL means top or end of list.

typedef struct msecnode_s
{
	sector_t *m_sector; // a sector containing this object
	struct mobj_s *m_thing;  // this object
	struct msecnode_s *m_sectorlist_prev;  // prev msecnode_t for this thing
	struct msecnode_s *m_sectorlist_next;  // next msecnode_t for this thing
	struct msecnode_s *m_thinglist_prev;  // prev msecnode_t for this sector
	struct msecnode_s *m_thinglist_next;  // next msecnode_t for this sector
	boolean visited; // used in search algorithms
} msecnode_t;

typedef struct mprecipsecnode_s
{
	sector_t *m_sector; // a sector containing this object
	struct precipmobj_s *m_thing;  // this object
	struct mprecipsecnode_s *m_sectorlist_prev;  // prev msecnode_t for this thing
	struct mprecipsecnode_s *m_sectorlist_next;  // next msecnode_t for this thing
	struct mprecipsecnode_s *m_thinglist_prev;  // prev msecnode_t for this sector
	struct mprecipsecnode_s *m_thinglist_next;  // next msecnode_t for this sector
	boolean visited; // used in search algorithms
} mprecipsecnode_t;

// for now, only used in hardware mode
// maybe later for software as well?
// that's why it's moved here
typedef struct light_s
{
	UINT16 type;          // light,... (cfr #define in hwr_light.c)

	float light_xoffset;
	float light_yoffset;  // y offset to adjust corona's height

	UINT32 corona_color;   // color of the light for static lighting
	float corona_radius;  // radius of the coronas

	UINT32 dynamic_color;  // color of the light for dynamic lighting
	float dynamic_radius; // radius of the light ball
	float dynamic_sqrradius; // radius^2 of the light ball
} light_t;

typedef struct lightmap_s
{
	float s[2], t[2];
	light_t *light;
	struct lightmap_s *next;
} lightmap_t;

//
// The lineseg.
//
typedef struct seg_s
{
	vertex_t *v1;
	vertex_t *v2;

	INT32 side;

	fixed_t offset;

	angle_t angle;

	side_t *sidedef;
	line_t *linedef;

	// Sector references.
	// Could be retrieved from linedef, too. backsector is NULL for one sided lines
	sector_t *frontsector;
	sector_t *backsector;

	fixed_t length;	// precalculated seg length
#ifdef HWRENDER
	// new pointers so that AdjustSegs doesn't mess with v1/v2
	void *pv1; // polyvertex_t
	void *pv2; // polyvertex_t
	float flength; // length of the seg, used by hardware renderer

	lightmap_t *lightmaps; // for static lightmap
#endif

	// Why slow things down by calculating lightlists for every thick side?
	size_t numlights;
	r_lightlist_t *rlights;
	polyobj_t *polyseg;
	boolean dontrenderme;
	boolean glseg;
} seg_t;

//
// BSP node.
//
typedef struct
{
	// Partition line.
	fixed_t x, y;
	fixed_t dx, dy;

	// Bounding box for each child.
	fixed_t bbox[2][4];

	// If NF_SUBSECTOR its a subsector.
	UINT16 children[2];
} node_t;

#if defined(_MSC_VER)
#pragma pack(1)
#endif

// posts are runs of non masked source pixels
typedef struct
{
	UINT8 topdelta; // -1 is the last post in a column
	UINT8 length;   // length data bytes follows
} ATTRPACK doompost_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

typedef struct
{
	unsigned topdelta;
	unsigned length;
	size_t data_offset;
} post_t;

// column_t is a list of 0 or more post_t
typedef struct
{
	unsigned num_posts;
	post_t *posts;
	UINT8 *pixels;
} column_t;

//
// OTHER TYPES
//

#ifndef MAXFFLOORS
#define MAXFFLOORS 40
#endif

//
// ?
//
typedef struct drawseg_s
{
	seg_t *curline;
	INT32 x1;
	INT32 x2;

	fixed_t scale1;
	fixed_t scale2;
	fixed_t scalestep;

	INT32 silhouette; // 0 = none, 1 = bottom, 2 = top, 3 = both

	fixed_t bsilheight; // do not clip sprites above this
	fixed_t tsilheight; // do not clip sprites below this

	fixed_t offsetx;

	// Pointers to lists for sprite clipping, all three adjusted so [x1] is first value.
	INT16 *sprtopclip;
	INT16 *sprbottomclip;
	fixed_t *maskedtexturecol;
	fixed_t *invscale;

	struct visplane_s *ffloorplanes[MAXFFLOORS];
	INT32 numffloorplanes;
	struct ffloor_s *thicksides[MAXFFLOORS];
	fixed_t *thicksidecol;
	INT32 numthicksides;
	fixed_t frontscale[MAXVIDWIDTH];

	UINT8 portalpass; // if > 0 and <= portalrender, do not affect sprite clipping

	fixed_t maskedtextureheight[MAXVIDWIDTH]; // For handling sloped midtextures

	vertex_t leftpos, rightpos; // Used for rendering FOF walls with slopes
} drawseg_t;

typedef enum
{
	PALETTE         = 0,  // 1 byte is the index in the doom palette (as usual)
	INTENSITY       = 1,  // 1 byte intensity
	INTENSITY_ALPHA = 2,  // 2 byte: alpha then intensity
	RGB24           = 3,  // 24 bit rgb
	RGBA32          = 4,  // 32 bit rgba
} pic_mode_t;

#ifdef ROTSPRITE
typedef struct
{
	INT32 angles;
	void **patches;
} rotsprite_t;
#endif

// Patches.
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures, and we compose
// textures from the TEXTURES list of patches.
//
typedef struct
{
	INT16 width, height;
	INT16 leftoffset, topoffset;

	UINT8 *pixels;
	column_t *columns;
	post_t *posts;

	void *hardware; // OpenGL patch, allocated whenever necessary
	void *flats[4]; // The patch as flats

#ifdef ROTSPRITE
	rotsprite_t *rotated; // Rotated patches
#endif
} patch_t;

#if defined(_MSC_VER)
#pragma pack(1)
#endif

typedef struct
{
	INT16 width;          // bounding box size
	INT16 height;
	INT16 leftoffset;     // pixels to the left of origin
	INT16 topoffset;      // pixels below the origin
	INT32 columnofs[8];     // only [width] used
	// the [0] is &columnofs[width]
} ATTRPACK softwarepatch_t;

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#endif

// a pic is an unmasked block of pixels, stored in horizontal way
typedef struct
{
	INT16 width;
	UINT8 zero;       // set to 0 allow autodetection of pic_t
	                 // mode instead of patch or raw
	UINT8 mode;       // see pic_mode_t above
	INT16 height;
	INT16 reserved1; // set to 0
	UINT8 data[0];
} ATTRPACK pic_t;

#ifdef _MSC_VER
#pragma warning(default : 4200)
#endif

#if defined(_MSC_VER)
#pragma pack()
#endif

// Possible alpha types for a patch.
enum patchalphastyle {AST_COPY, AST_TRANSLUCENT, AST_ADD, AST_SUBTRACT, AST_REVERSESUBTRACT, AST_MODULATE, AST_OVERLAY, AST_FOG};

typedef enum
{
	RF_HORIZONTALFLIP   = 0x0001,   // Flip sprite horizontally
	RF_VERTICALFLIP     = 0x0002,   // Flip sprite vertically
	RF_ABSOLUTEOFFSETS  = 0x0004,   // Sprite uses the object's offsets absolutely, instead of relatively
	RF_FLIPOFFSETS      = 0x0008,   // Relative object offsets are flipped with the sprite

	RF_SPLATMASK        = 0x00F0,   // --Floor sprite flags
	RF_SLOPESPLAT       = 0x0010,   // Rotate floor sprites by a slope
	RF_OBJECTSLOPESPLAT = 0x0020,   // Rotate floor sprites by the object's standing slope
	RF_NOSPLATBILLBOARD = 0x0040,   // Don't billboard floor sprites (faces forward from the view angle)
	RF_NOSPLATROLLANGLE = 0x0080,   // Don't rotate floor sprites by the object's rollangle (uses rotated patches instead)

	RF_BRIGHTMASK       = 0x00000300,   // --Bright modes
	RF_FULLBRIGHT       = 0x00000100,   // Sprite is drawn at full brightness
	RF_FULLDARK         = 0x00000200,   // Sprite is drawn completely dark
	RF_SEMIBRIGHT       = (RF_FULLBRIGHT | RF_FULLDARK), // between sector bright and full bright

	RF_NOCOLORMAPS      = 0x00000400,   // Sprite is not drawn with colormaps

	RF_SPRITETYPEMASK   = 0x00003000,   // --Different sprite types
	RF_PAPERSPRITE      = 0x00001000,   // Paper sprite
	RF_FLOORSPRITE      = 0x00002000,   // Floor sprite

	RF_SHADOWDRAW       = 0x00004000,  // Stretches and skews the sprite like a shadow.
	RF_SHADOWEFFECTS    = 0x00008000,  // Scales and becomes transparent like a shadow.
	RF_DROPSHADOW       = (RF_SHADOWDRAW | RF_SHADOWEFFECTS | RF_FULLDARK),
} renderflags_t;

typedef enum
{
	SRF_SINGLE      = 0,   // 0-angle for all rotations
	SRF_3D          = 1,   // Angles 1-8
	SRF_3DGE        = 2,   // 3DGE, ZDoom and Doom Legacy all have 16-angle support. Why not us?
	SRF_3DMASK      = SRF_3D|SRF_3DGE, // 3
	SRF_LEFT        = 4,   // Left side uses single patch
	SRF_RIGHT       = 8,   // Right side uses single patch
	SRF_2D          = SRF_LEFT|SRF_RIGHT, // 12
	SRF_NONE        = 0xff // Initial value
} spriterotateflags_t;     // SRF's up!

//
// Sprites are patches with a special naming convention so they can be
//  recognized by R_InitSprites.
// The base name is NNNNFx or NNNNFxFx, with x indicating the rotation,
//  x = 0, 1-8, 9+A-G, L/R
// The sprite and frame specified by a thing_t is range checked at run time.
// A sprite is a patch_t that is assumed to represent a three dimensional
//  object and may have multiple rotations predrawn.
// Horizontal flipping is used to save space, thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used for all views: NNNNF0
// Some sprites will take the entirety of the left side: NNNNFL
// Or the right side: NNNNFR
// Or both, mirrored: NNNNFLFR
//
typedef struct
{
	// If false use 0 for any position.
	// Note: as eight entries are available, we might as well insert the same
	//  name eight times.
	UINT8 rotate; // see spriterotateflags_t above

	// Lump to use for view angles 0-7/15.
	lumpnum_t lumppat[16]; // lump number 16 : 16 wad : lump
	size_t lumpid[16]; // id in the spriteoffset, spritewidth, etc. tables

	// Flip bits (1 = flip) to use for view angles 0-7/15.
	UINT16 flip;

#ifdef ROTSPRITE
	rotsprite_t *rotated[16]; // Rotated patches
#endif
} spriteframe_t;

//
// A sprite definition:  a number of animation frames.
//
typedef struct
{
	size_t numframes;
	spriteframe_t *spriteframes;
} spritedef_t;

#endif
