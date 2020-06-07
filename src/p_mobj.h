// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_mobj.h
/// \brief Map Objects, MObj, definition and handling

#ifndef __P_MOBJ__
#define __P_MOBJ__

// Basics.
#include "tables.h"
#include "m_fixed.h"

// We need the thinker_t stuff.
#include "d_think.h"

// We need the WAD data structure for Map things, from the THINGS lump.
#include "doomdata.h"

// States are tied to finite states are tied to animation frames.
// Needs precompiled tables/data structures.
#include "info.h"

//
// NOTES: mobj_t
//
// mobj_ts are used to tell the refresh where to draw an image,
// tell the world simulation when objects are contacted,
// and tell the sound driver how to position a sound.
//
// The refresh uses the next and prev links to follow
// lists of things in sectors as they are being drawn.
// The sprite, frame, and angle elements determine which patch_t
// is used to draw the sprite if it is visible.
// The sprite and frame values are allmost allways set
// from state_t structures.
// The statescr.exe utility generates the states.h and states.c
// files that contain the sprite/frame numbers from the
// statescr.txt source file.
// The xyz origin point represents a point at the bottom middle
// of the sprite (between the feet of a biped).
// This is the default origin position for patch_ts grabbed
// with lumpy.exe.
// A walking creature will have its z equal to the floor
// it is standing on.
//
// The sound code uses the x,y, and subsector fields
// to do stereo positioning of any sound effited by the mobj_t.
//
// The play simulation uses the blocklinks, x,y,z, radius, height
// to determine when mobj_ts are touching each other,
// touching lines in the map, or hit by trace lines (gunshots,
// lines of sight, etc).
// The mobj_t->flags element has various bit flags
// used by the simulation.
//
// Every mobj_t is linked into a single sector
// based on its origin coordinates.
// The subsector_t is found with R_PointInSubsector(x,y),
// and the sector_t can be found with subsector->sector.
// The sector links are only used by the rendering code,
// the play simulation does not care about them at all.
//
// Any mobj_t that needs to be acted upon by something else
// in the play world (block movement, be shot, etc) will also
// need to be linked into the blockmap.
// If the thing has the MF_NOBLOCK flag set, it will not use
// the block links. It can still interact with other things,
// but only as the instigator (missiles will run into other
// things, but nothing can run into a missile).
// Each block in the grid is 128*128 units, and knows about
// every line_t that it contains a piece of, and every
// interactable mobj_t that has its origin contained.
//
// A valid mobj_t is a mobj_t that has the proper subsector_t
// filled in for its xy coordinates and is linked into the
// sector from which the subsector was made, or has the
// MF_NOSECTOR flag set (the subsector_t needs to be valid
// even if MF_NOSECTOR is set), and is linked into a blockmap
// block or has the MF_NOBLOCKMAP flag set.
// Links should only be modified by the P_[Un]SetThingPosition()
// functions.
// Do not change the MF_NO? flags while a thing is valid.
//
// Any questions?
//

//
// Misc. mobj flags
//
typedef enum
{
	// Call P_TouchSpecialThing when touched.
	MF_SPECIAL          = 1,
	// Blocks.
	MF_SOLID            = 1<<1,
	// Can be hit.
	MF_SHOOTABLE        = 1<<2,
	// Don't use the sector links (invisible but touchable).
	MF_NOSECTOR         = 1<<3,
	// Don't use the blocklinks (inert but displayable)
	MF_NOBLOCKMAP       = 1<<4,
	// Thin, paper-like collision bound (for visual equivalent, see FF_PAPERSPRITE)
	MF_PAPERCOLLISION            = 1<<5,
	// You can push this object. It can activate switches and things by pushing it on top.
	MF_PUSHABLE         = 1<<6,
	// Object is a boss.
	MF_BOSS             = 1<<7,
	// On level spawning (initial position), hang from ceiling instead of stand on floor.
	MF_SPAWNCEILING     = 1<<8,
	// Don't apply gravity (every tic); object will float, keeping current height
	//  or changing it actively.
	MF_NOGRAVITY        = 1<<9,
	// This object is an ambient sound.
	MF_AMBIENT          = 1<<10,
	// Slide this object when it hits a wall.
	MF_SLIDEME          = 1<<11,
	// Player cheat.
	MF_NOCLIP           = 1<<12,
	// Allow moves to any height, no gravity. For active floaters.
	MF_FLOAT            = 1<<13,
	// Monitor powerup icon. These rise a bit.
	MF_BOXICON          = 1<<14,
	// Don't hit same species, explode on block.
	// Player missiles as well as fireballs of various kinds.
	MF_MISSILE          = 1<<15,
	// Item is a spring.
	MF_SPRING           = 1<<16,
	// Bounce off walls and things.
	MF_BOUNCE           = 1<<17,
	// Item box
	MF_MONITOR          = 1<<18,
	// Don't run the thinker for this object.
	MF_NOTHINK          = 1<<19,
	// Fire object. Doesn't harm if you have fire shield.
	MF_FIRE             = 1<<20,
	// Don't adjust z if below or above floorz/ceilingz
	MF_NOCLIPHEIGHT     = 1<<21,
	// This mobj is an enemy!
	MF_ENEMY            = 1<<22,
	// Scenery (uses scenery thinker).
	MF_SCENERY          = 1<<23,
	// Painful (shit hurts).
	MF_PAIN             = 1<<24,
	// This mobj will stick to any surface or solid object it touches.
	MF_STICKY           = 1<<25,
	// NiGHTS hidden item. Goes to seestate and turns MF_SPECIAL when paralooped.
	MF_NIGHTSITEM       = 1<<26,
	// for chase camera, don't be blocked by things (partial clipping)
	MF_NOCLIPTHING      = 1<<27,
	// Missile bounces like a grenade.
	MF_GRENADEBOUNCE    = 1<<28,
	// Run the action thinker on spawn.
	MF_RUNSPAWNFUNC     = 1<<29,
	// free: 1<<30 and 1<<31
} mobjflag_t;

typedef enum
{
	MF2_AXIS           = 1,     // It's a NiGHTS axis! (For faster checking)
	MF2_TWOD           = 1<<1,  // Moves like it's in a 2D level
	MF2_DONTRESPAWN    = 1<<2,  // Don't respawn this object!
	MF2_DONTDRAW       = 1<<3,  // Don't generate a vissprite
	MF2_AUTOMATIC      = 1<<4,  // Thrown ring has automatic properties
	MF2_RAILRING       = 1<<5,  // Thrown ring has rail properties
	MF2_BOUNCERING     = 1<<6,  // Thrown ring has bounce properties
	MF2_EXPLOSION      = 1<<7,  // Thrown ring has explosive properties
	MF2_SCATTER        = 1<<8,  // Thrown ring has scatter properties
	MF2_BEYONDTHEGRAVE = 1<<9,  // Source of this missile has died and has since respawned.
	MF2_SLIDEPUSH      = 1<<10, // MF_PUSHABLE that pushes continuously.
	MF2_CLASSICPUSH    = 1<<11, // Drops straight down when object has negative momz.
	MF2_INVERTAIMABLE  = 1<<12, // Flips whether it's targetable by A_LookForEnemies (enemies no, decoys yes)
	MF2_INFLOAT        = 1<<13, // Floating to a height for a move, don't auto float to target's height.
	MF2_DEBRIS         = 1<<14, // Splash ring from explosion ring
	MF2_NIGHTSPULL     = 1<<15, // Attracted from a paraloop
	MF2_JUSTATTACKED   = 1<<16, // can be pushed by other moving mobjs
	MF2_FIRING         = 1<<17, // turret fire
	MF2_SUPERFIRE      = 1<<18, // Firing something with Super Sonic-stopping properties. Or, if mobj has MF_MISSILE, this is the actual fire from it.
	MF2_SHADOW         = 1<<19, // Fuzzy draw, makes targeting harder.
	MF2_STRONGBOX      = 1<<20, // Flag used for "strong" random monitors.
	MF2_OBJECTFLIP     = 1<<21, // Flag for objects that always have flipped gravity.
	MF2_SKULLFLY       = 1<<22, // Special handling: skull in flight.
	MF2_FRET           = 1<<23, // Flashing from a previous hit
	MF2_BOSSNOTRAP     = 1<<24, // No Egg Trap after boss
	MF2_BOSSFLEE       = 1<<25, // Boss is fleeing!
	MF2_BOSSDEAD       = 1<<26, // Boss is dead! (Not necessarily fleeing, if a fleeing point doesn't exist.)
	MF2_AMBUSH         = 1<<27, // Alternate behaviour typically set by MTF_AMBUSH
	MF2_LINKDRAW       = 1<<28, // Draw vissprite of mobj immediately before/after tracer's vissprite (dependent on dispoffset and position)
	MF2_SHIELD         = 1<<29, // Thinker calls P_AddShield/P_ShieldLook (must be partnered with MF_SCENERY to use)
	// free: to and including 1<<31
} mobjflag2_t;

typedef enum
{
	DI_NODIR = -1,
	DI_EAST = 0,
	DI_NORTHEAST = 1,
	DI_NORTH = 2,
	DI_NORTHWEST = 3,
	DI_WEST = 4,
	DI_SOUTHWEST = 5,
	DI_SOUTH = 6,
	DI_SOUTHEAST = 7,
	NUMDIRS = 8,
} dirtype_t;

//
// Mobj extra flags
//
typedef enum
{
	// The mobj stands on solid floor (not on another mobj or in air)
	MFE_ONGROUND          = 1,
	// The mobj just hit the floor while falling, this is cleared on next frame
	// (instant damage in lava/slime sectors to prevent jump cheat..)
	MFE_JUSTHITFLOOR      = 1<<1,
	// The mobj stands in a sector with water, and touches the surface
	// this bit is set once and for all at the start of mobjthinker
	MFE_TOUCHWATER        = 1<<2,
	// The mobj stands in a sector with water, and his waist is BELOW the water surface
	// (for player, allows swimming up/down)
	MFE_UNDERWATER        = 1<<3,
	// used for ramp sectors
	MFE_JUSTSTEPPEDDOWN   = 1<<4,
	// Vertically flip sprite/allow upside-down physics
	MFE_VERTICALFLIP      = 1<<5,
	// Goo water
	MFE_GOOWATER          = 1<<6,
	// The mobj is touching a lava block
	MFE_TOUCHLAVA         = 1<<7,
	// Mobj was already pushed this tic
	MFE_PUSHED            = 1<<8,
	// Mobj was already sprung this tic
	MFE_SPRUNG            = 1<<9,
	// Platform movement
	MFE_APPLYPMOMZ        = 1<<10,
	// Compute and trigger on mobj angle relative to tracer
	// See Linedef Exec 457 (Track mobj angle to point)
	MFE_TRACERANGLE       = 1<<11,
	// free: to and including 1<<15
} mobjeflag_t;

//
// PRECIPITATION flags ?! ?! ?!
//
typedef enum {
	// Don't draw.
	PCF_INVISIBLE = 1,
	// Above pit.
	PCF_PIT = 2,
	// Above FOF.
	PCF_FOF = 4,
	// Above MOVING FOF (this means we need to keep floorz up to date...)
	PCF_MOVINGFOF = 8,
	// Is rain.
	PCF_RAIN = 16,
	// Ran the thinker this tic.
	PCF_THUNK = 32,
} precipflag_t;
// Map Object definition.
typedef struct mobj_s
{
	// List: thinker links.
	thinker_t thinker;

	// Info for drawing: position.
	fixed_t x, y, z;

	// More list: links in sector (if needed)
	struct mobj_s *snext;
	struct mobj_s **sprev; // killough 8/11/98: change to ptr-to-ptr

	// More drawing info: to determine current sprite.
	angle_t angle;  // orientation
	angle_t rollangle;
	spritenum_t sprite; // used to find patch_t and flip value
	UINT32 frame; // frame number, plus bits see p_pspr.h
	UINT8 sprite2; // player sprites
	UINT16 anim_duration; // for FF_ANIMATE states

	struct msecnode_s *touching_sectorlist; // a linked list of sectors where this object appears

	struct subsector_s *subsector; // Subsector the mobj resides in.

	// The closest interval over all contacted sectors (or things).
	fixed_t floorz; // Nearest floor below.
	fixed_t ceilingz; // Nearest ceiling above.
	struct ffloor_s *floorrover; // FOF referred by floorz
	struct ffloor_s *ceilingrover; // FOF referred by ceilingz

	// For movement checking.
	fixed_t radius;
	fixed_t height;

	// Momentums, used to update position.
	fixed_t momx, momy, momz;
	fixed_t pmomz; // If you're on a moving floor, its "momz" would be here

	INT32 tics; // state tic counter
	state_t *state;
	UINT32 flags; // flags from mobjinfo tables
	UINT32 flags2; // MF2_ flags
	UINT16 eflags; // extra flags

	void *skin; // overrides 'sprite' when non-NULL (for player bodies to 'remember' the skin)
	// Player and mobj sprites in multiplayer modes are modified
	//  using an internal color lookup table for re-indexing.
	UINT16 color; // This replaces MF_TRANSLATION. Use 0 for default (no translation).

	// Interaction info, by BLOCKMAP.
	// Links in blocks (if needed).
	struct mobj_s *bnext;
	struct mobj_s **bprev; // killough 8/11/98: change to ptr-to-ptr

	// Additional pointers for NiGHTS hoops
	struct mobj_s *hnext;
	struct mobj_s *hprev;

	mobjtype_t type;
	const mobjinfo_t *info; // &mobjinfo[mobj->type]

	INT32 health; // for player this is rings + 1 -- no it isn't, not any more!!

	// Movement direction, movement generation (zig-zagging).
	angle_t movedir; // dirtype_t 0-7; also used by Deton for up/down angle
	INT32 movecount; // when 0, select a new dir

	struct mobj_s *target; // Thing being chased/attacked (or NULL), and originator for missiles.

	INT32 reactiontime; // If not 0, don't attack yet.

	INT32 threshold; // If >0, the target will be chased no matter what.

	// Additional info record for player avatars only.
	// Only valid if type == MT_PLAYER
	struct player_s *player;

	INT32 lastlook; // Player number last looked for.

	mapthing_t *spawnpoint; // Used for CTF flags, objectplace, and a handful other applications.

	struct mobj_s *tracer; // Thing being chased/attacked for tracers.

	fixed_t friction;
	fixed_t movefactor;

	INT32 fuse; // Does something in P_MobjThinker on reaching 0.
	fixed_t watertop; // top of the water FOF the mobj is in
	fixed_t waterbottom; // bottom of the water FOF the mobj is in

	UINT32 mobjnum; // A unique number for this mobj. Used for restoring pointers on save games.

	fixed_t scale;
	fixed_t destscale;
	fixed_t scalespeed;

	// Extra values are for internal use for whatever you want
	INT32 extravalue1;
	INT32 extravalue2;

	// Custom values are not to be altered by us!
	// They are for SOCs to store things in.
	INT32 cusval;
	INT32 cvmem;

	struct pslope_s *standingslope; // The slope that the object is standing on (shouldn't need synced in savegames, right?)

	boolean colorized; // Whether the mobj uses the rainbow colormap
	fixed_t shadowscale; // If this object casts a shadow, and the size relative to radius

	// WARNING: New fields must be added separately to savegame and Lua.
} mobj_t;

//
// For precipitation
//
// Sometimes this is casted to a mobj_t,
// so please keep the start of the
// structure the same.
//
typedef struct precipmobj_s
{
	// List: thinker links.
	thinker_t thinker;

	// Info for drawing: position.
	fixed_t x, y, z;

	// More list: links in sector (if needed)
	struct precipmobj_s *snext;
	struct precipmobj_s **sprev; // killough 8/11/98: change to ptr-to-ptr

	// More drawing info: to determine current sprite.
	angle_t angle;  // orientation
	angle_t rollangle;
	spritenum_t sprite; // used to find patch_t and flip value
	UINT32 frame; // frame number, plus bits see p_pspr.h
	UINT8 sprite2; // player sprites
	UINT16 anim_duration; // for FF_ANIMATE states

	struct mprecipsecnode_s *touching_sectorlist; // a linked list of sectors where this object appears

	struct subsector_s *subsector; // Subsector the mobj resides in.

	// The closest interval over all contacted sectors (or things).
	fixed_t floorz; // Nearest floor below.
	fixed_t ceilingz; // Nearest ceiling above.
	struct ffloor_s *floorrover; // FOF referred by floorz
	struct ffloor_s *ceilingrover; // FOF referred by ceilingz

	// For movement checking.
	fixed_t radius; // Fixed at 2*FRACUNIT
	fixed_t height; // Fixed at 4*FRACUNIT

	// Momentums, used to update position.
	fixed_t momx, momy, momz;
	fixed_t precipflags; // fixed_t so it uses the same spot as "pmomz" even as we use precipflags_t for it

	INT32 tics; // state tic counter
	state_t *state;
	INT32 flags; // flags from mobjinfo tables
} precipmobj_t;

typedef struct actioncache_s
{
	struct actioncache_s *next;
	struct actioncache_s *prev;
	struct mobj_s *mobj;
	INT32 statenum;
} actioncache_t;

extern actioncache_t actioncachehead;

void P_InitCachedActions(void);
void P_RunCachedActions(void);
void P_AddCachedAction(mobj_t *mobj, INT32 statenum);

// check mobj against water content, before movement code
void P_MobjCheckWater(mobj_t *mobj);

// Player spawn points
void P_SpawnPlayer(INT32 playernum);
void P_MovePlayerToSpawn(INT32 playernum, mapthing_t *mthing);
void P_MovePlayerToStarpost(INT32 playernum);
void P_AfterPlayerSpawn(INT32 playernum);

fixed_t P_GetMobjSpawnHeight(const mobjtype_t mobjtype, const fixed_t x, const fixed_t y, const fixed_t offset, const boolean flip);
fixed_t P_GetMapThingSpawnHeight(const mobjtype_t mobjtype, const mapthing_t* mthing, const fixed_t x, const fixed_t y);

mobj_t *P_SpawnMapThing(mapthing_t *mthing);
void P_SpawnHoop(mapthing_t *mthing);
void P_SetBonusTime(mobj_t *mobj);
void P_SpawnItemPattern(mapthing_t *mthing, boolean bonustime);
void P_SpawnHoopOfSomething(fixed_t x, fixed_t y, fixed_t z, fixed_t radius, INT32 number, mobjtype_t type, angle_t rotangle);
void P_SpawnPrecipitation(void);
void P_SpawnParaloop(fixed_t x, fixed_t y, fixed_t z, fixed_t radius, INT32 number, mobjtype_t type, statenum_t nstate, angle_t rotangle, boolean spawncenter);
boolean P_BossTargetPlayer(mobj_t *actor, boolean closest);
boolean P_SupermanLook4Players(mobj_t *actor);
void P_DestroyRobots(void);
void P_SnowThinker(precipmobj_t *mobj);
void P_RainThinker(precipmobj_t *mobj);
void P_NullPrecipThinker(precipmobj_t *mobj);
void P_RemovePrecipMobj(precipmobj_t *mobj);
void P_SetScale(mobj_t *mobj, fixed_t newscale);
void P_XYMovement(mobj_t *mo);
void P_EmeraldManager(void);

extern INT32 modulothing;

#define MAXHUNTEMERALDS 64
extern mapthing_t *huntemeralds[MAXHUNTEMERALDS];
extern INT32 numhuntemeralds;
extern boolean runemeraldmanager;
extern UINT16 emeraldspawndelay;
extern INT32 numstarposts;
extern UINT16 bossdisabled;
extern boolean stoppedclock;
#endif
