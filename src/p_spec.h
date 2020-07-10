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
/// \file  p_spec.h
/// \brief Implements special effects:
///        Texture animation, height or lighting changes
///        according to adjacent sectors, respective
///        utility functions, etc.

#ifndef __P_SPEC__
#define __P_SPEC__

extern mobj_t *skyboxmo[2]; // current skybox mobjs: 0 = viewpoint, 1 = centerpoint
extern mobj_t *skyboxviewpnts[16]; // array of MT_SKYBOX viewpoint mobjs
extern mobj_t *skyboxcenterpnts[16]; // array of MT_SKYBOX centerpoint mobjs

// GETSECSPECIAL (specialval, section)
//
// Pulls out the special # from a particular section.
//
#define GETSECSPECIAL(i,j) ((i >> ((j-1)*4))&15)

// This must be updated whenever we up the max flat size - quicker to assume rather than figuring out the sqrt of the specific flat's filesize.
#define MAXFLATSIZE (2048<<FRACBITS)

// at game start
void P_InitPicAnims(void);

// at map load (sectors)
void P_SetupLevelFlatAnims(void);

// at map load
void P_InitSpecials(void);
void P_SpawnSpecials(boolean fromnetsave);

// every tic
void P_UpdateSpecials(void);
sector_t *P_PlayerTouchingSectorSpecial(player_t *player, INT32 section, INT32 number);
void P_PlayerInSpecialSector(player_t *player);
void P_ProcessSpecialSector(player_t *player, sector_t *sector, sector_t *roversector);

fixed_t P_FindLowestFloorSurrounding(sector_t *sec);
fixed_t P_FindHighestFloorSurrounding(sector_t *sec);

fixed_t P_FindNextHighestFloor(sector_t *sec, fixed_t currentheight);
fixed_t P_FindNextLowestFloor(sector_t *sec, fixed_t currentheight);

fixed_t P_FindLowestCeilingSurrounding(sector_t *sec);
fixed_t P_FindHighestCeilingSurrounding(sector_t *sec);

INT32 P_FindMinSurroundingLight(sector_t *sector, INT32 max);

void P_SetupSignExit(player_t *player);
boolean P_IsFlagAtBase(mobjtype_t flag);

void P_SwitchWeather(INT32 weathernum);

boolean P_RunTriggerLinedef(line_t *triggerline, mobj_t *actor, sector_t *caller);
void P_LinedefExecute(INT16 tag, mobj_t *actor, sector_t *caller);
void P_RunNightserizeExecutors(mobj_t *actor);
void P_RunDeNightserizeExecutors(mobj_t *actor);
void P_RunNightsLapExecutors(mobj_t *actor);
void P_RunNightsCapsuleTouchExecutors(mobj_t *actor, boolean entering, boolean enoughspheres);

UINT16 P_GetFFloorID(ffloor_t *fflr);
ffloor_t *P_GetFFloorByID(sector_t *sec, UINT16 id);

//
// P_LIGHTS
//
/** Fire flicker action structure.
  */
typedef struct
{
	thinker_t thinker; ///< The thinker in use for the effect.
	sector_t *sector;  ///< The sector where action is taking place.
	INT32 count;
	INT32 resetcount;
	INT32 maxlight;    ///< The brightest light level to use.
	INT32 minlight;    ///< The darkest light level to use.
} fireflicker_t;

typedef struct
{
	thinker_t thinker;
	sector_t *sector;
	INT32 maxlight;
	INT32 minlight;
} lightflash_t;

/** Laser block thinker.
  */
typedef struct
{
	thinker_t thinker; ///< Thinker structure for laser.
	INT16 tag;
	line_t *sourceline;
	UINT8 nobosses;
} laserthink_t;

/** Strobe light action structure..
  */
typedef struct
{
	thinker_t thinker; ///< The thinker in use for the effect.
	sector_t *sector;  ///< The sector where the action is taking place.
	INT32 count;
	INT32 minlight;    ///< The minimum light level to use.
	INT32 maxlight;    ///< The maximum light level to use.
	INT32 darktime;    ///< How INT32 to use minlight.
	INT32 brighttime;  ///< How INT32 to use maxlight.
} strobe_t;

typedef struct
{
	thinker_t thinker;
	sector_t *sector;
	INT32 minlight;
	INT32 maxlight;
	INT32 direction;
	INT32 speed;
} glow_t;

/** Thinker struct for fading lights.
  */
typedef struct
{
	thinker_t thinker;		///< Thinker in use for the effect.
	sector_t *sector;		///< Sector where action is taking place.
	INT16 sourcelevel;		///< Light level we're fading from.
	INT16 destlevel;		///< Light level we're fading to.

	fixed_t fixedcurlevel;	///< Fixed point for current light level.
	fixed_t fixedpertic;	///< Fixed point for increment per tic.
	// The reason for those two above to be fixed point is to deal with decimal values that would otherwise get trimmed away.
	INT32 timer;			///< Internal timer.
} lightlevel_t;

#define GLOWSPEED 8
#define STROBEBRIGHT 5
#define FASTDARK 15
#define SLOWDARK 35

void P_RemoveLighting(sector_t *sector);

void T_FireFlicker(fireflicker_t *flick);
fireflicker_t *P_SpawnAdjustableFireFlicker(sector_t *minsector, sector_t *maxsector, INT32 length);
void T_LightningFlash(lightflash_t *flash);
void T_StrobeFlash(strobe_t *flash);

void P_SpawnLightningFlash(sector_t *sector);
strobe_t * P_SpawnAdjustableStrobeFlash(sector_t *minsector, sector_t *maxsector, INT32 darktime, INT32 brighttime, boolean inSync);

void T_Glow(glow_t *g);
glow_t *P_SpawnAdjustableGlowingLight(sector_t *minsector, sector_t *maxsector, INT32 length);

void P_FadeLightBySector(sector_t *sector, INT32 destvalue, INT32 speed, boolean ticbased);
void P_FadeLight(INT16 tag, INT32 destvalue, INT32 speed, boolean ticbased, boolean force);
void T_LightFade(lightlevel_t *ll);

typedef enum
{
	floor_special,
	ceiling_special,
	lighting_special,
} special_e;

//
// P_CEILNG
//
typedef enum
{
	raiseToHighest,
	lowerToLowest,
	raiseToLowest,
	lowerToLowestFast,

	instantRaise, // instant-move for ceilings

	lowerAndCrush,
	crushAndRaise,
	fastCrushAndRaise,
	crushCeilOnce,
	crushBothOnce,

	moveCeilingByFrontSector,
	instantMoveCeilingByFrontSector,

	moveCeilingByFrontTexture,

	bounceCeiling,
	bounceCeilingCrush,
} ceiling_e;

/** Ceiling movement structure.
  */
typedef struct
{
	thinker_t thinker;    ///< Thinker for the type of movement.
	ceiling_e type;       ///< Type of movement.
	sector_t *sector;     ///< Sector where the action is taking place.
	fixed_t bottomheight; ///< The lowest height to move to.
	fixed_t topheight;    ///< The highest height to move to.
	fixed_t speed;        ///< Ceiling speed.
	fixed_t oldspeed;
	fixed_t delay;
	fixed_t delaytimer;
	UINT8 crush;           ///< Whether to crush things or not.

	INT32 texture;        ///< The number of a flat to use when done.
	INT32 direction;      ///< 1 = up, 0 = waiting, -1 = down.

	// ID
	INT32 tag;
	INT32 olddirection;
	fixed_t origspeed;    ///< The original, "real" speed.
	INT32 sourceline;     ///< Index of the source linedef
} ceiling_t;

#define CEILSPEED (FRACUNIT)

INT32 EV_DoCeiling(line_t *line, ceiling_e type);

INT32 EV_DoCrush(line_t *line, ceiling_e type);
void T_CrushCeiling(ceiling_t *ceiling);

void T_MoveCeiling(ceiling_t *ceiling);

//
// P_FLOOR
//
typedef enum
{
	// lower floor to lowest surrounding floor
	lowerFloorToLowest,

	// raise floor to next highest surrounding floor
	raiseFloorToNearestFast,

	// move the floor down instantly
	instantLower,

	moveFloorByFrontSector,
	instantMoveFloorByFrontSector,

	moveFloorByFrontTexture,

	bounceFloor,
	bounceFloorCrush,

	crushFloorOnce,
} floor_e;

typedef enum
{
	elevateUp,
	elevateDown,
	elevateCurrent,
	elevateContinuous,
	elevateBounce,
	elevateHighest,
	bridgeFall,
} elevator_e;

typedef struct
{
	thinker_t thinker;
	floor_e type;
	UINT8 crush;
	sector_t *sector;
	INT32 direction;
	INT32 texture;
	fixed_t floordestheight;
	fixed_t speed;
	fixed_t origspeed;
	fixed_t delay;
	fixed_t delaytimer;
} floormove_t;

typedef struct
{
	thinker_t thinker;
	elevator_e type;
	sector_t *sector;
	sector_t *actionsector; // The sector the rover action is taking place in.
	INT32 direction;
	fixed_t floordestheight;
	fixed_t ceilingdestheight;
	fixed_t speed;
	fixed_t origspeed;
	fixed_t low;
	fixed_t high;
	fixed_t distance;
	fixed_t delay;
	fixed_t delaytimer;
	fixed_t floorwasheight; // Height the floor WAS at
	fixed_t ceilingwasheight; // Height the ceiling WAS at
	line_t *sourceline;
} elevator_t;

typedef enum
{
	CF_RETURN   = 1,    // Return after crumbling
	CF_FLOATBOB = 1<<1, // Float on water
	CF_REVERSE  = 1<<2, // Reverse gravity
} crumbleflag_t;

typedef struct
{
	thinker_t thinker;
	line_t *sourceline;
	sector_t *sector;
	sector_t *actionsector; // The sector the rover action is taking place in.
	player_t *player; // Player who initiated the thinker (used for airbob)
	INT32 direction;
	INT32 origalpha;
	INT32 timer;
	fixed_t speed;
	fixed_t floorwasheight; // Height the floor WAS at
	fixed_t ceilingwasheight; // Height the ceiling WAS at
	UINT8 flags;
} crumble_t;

typedef struct
{
	thinker_t thinker;
	line_t *sourceline; // Source line of the thinker
} noenemies_t;

typedef struct
{
	thinker_t thinker;
	sector_t *sector;
	fixed_t speed;
	INT32 direction;
	fixed_t floorstartheight;
	fixed_t ceilingstartheight;
	fixed_t destheight;
} continuousfall_t;

typedef struct
{
	thinker_t thinker;
	line_t *sourceline;
	sector_t *sector;
	fixed_t speed;
	fixed_t distance;
	fixed_t floorwasheight;
	fixed_t ceilingwasheight;
	boolean low;
} bouncecheese_t;

typedef struct
{
	thinker_t thinker;
	sector_t *sector;
	fixed_t speed;
	INT32 direction;
	fixed_t floorstartheight;
	fixed_t ceilingstartheight;
	INT16 tag;
} mariothink_t;

typedef struct
{
	thinker_t thinker;
	line_t *sourceline;
	sector_t *sector;
} mariocheck_t;

typedef struct
{
	thinker_t thinker;
	line_t *sourceline;
	sector_t *sector;
	fixed_t crushspeed;
	fixed_t retractspeed;
	INT32 direction;
	fixed_t floorstartheight;
	fixed_t ceilingstartheight;
	INT32 delay;
	INT16 tag;
	UINT16 sound;
} thwomp_t;

typedef struct
{
	thinker_t thinker;
	line_t *sourceline;
	sector_t *sector;
	INT16 tag;
} floatthink_t;

typedef struct
{
	thinker_t thinker;
	line_t *sourceline; // Source line of the thinker
	boolean playersInArea[MAXPLAYERS];
	boolean playersOnArea[MAXPLAYERS];
	boolean triggerOnExit;
} eachtime_t;

typedef enum
{
	RF_REVERSE  = 1,    //Lower when stood on
	RF_SPINDASH = 1<<1, //Require spindash to move
	RF_DYNAMIC  = 1<<2, //Dynamically sinking platform
} raiseflag_t;

typedef struct
{
	thinker_t thinker;
	INT16 tag;
	sector_t *sector;
	fixed_t ceilingbottom;
	fixed_t ceilingtop;
	fixed_t basespeed;
	fixed_t extraspeed; //For dynamically sinking platform
	UINT8 shaketimer; //For dynamically sinking platform
	UINT8 flags;
} raise_t;

#define ELEVATORSPEED (FRACUNIT*4)
#define FLOORSPEED (FRACUNIT)

typedef enum
{
	ok,
	crushed,
	pastdest
} result_e;

result_e T_MovePlane(sector_t *sector, fixed_t speed, fixed_t dest, boolean crush,
	boolean ceiling, INT32 direction);
void EV_DoFloor(line_t *line, floor_e floortype);
void EV_DoElevator(line_t *line, elevator_e elevtype, boolean customspeed);
void EV_CrumbleChain(sector_t *sec, ffloor_t *rover);
void EV_BounceSector(sector_t *sector, fixed_t momz, line_t *sourceline);

// Some other special 3dfloor types
INT32 EV_StartCrumble(sector_t *sector, ffloor_t *rover,
	boolean floating, player_t *player, fixed_t origalpha, boolean crumblereturn);

void EV_DoContinuousFall(sector_t *sec, sector_t *backsector, fixed_t spd, boolean backwards);

void EV_MarioBlock(ffloor_t *rover, sector_t *sector, mobj_t *puncher);

void T_MoveFloor(floormove_t *movefloor);

void T_MoveElevator(elevator_t *elevator);
void T_ContinuousFalling(continuousfall_t *faller);
void T_BounceCheese(bouncecheese_t *bouncer);
void T_StartCrumble(crumble_t *crumble);
void T_MarioBlock(mariothink_t *block);
void T_FloatSector(floatthink_t *floater);
void T_MarioBlockChecker(mariocheck_t *block);
void T_ThwompSector(thwomp_t *thwomp);
void T_NoEnemiesSector(noenemies_t *nobaddies);
void T_EachTimeThinker(eachtime_t *eachtime);
void T_CameraScanner(elevator_t *elevator);
void T_RaiseSector(raise_t *raise);

typedef struct
{
	thinker_t thinker; // Thinker for linedef executor delay
	line_t *line;      // Pointer to line that is waiting.
	mobj_t *caller;    // Pointer to calling mobj
	sector_t *sector;  // Pointer to triggering sector
	INT32 timer;       // Delay timer
} executor_t;

void T_ExecutorDelay(executor_t *e);

/** Generalized scroller.
  */
typedef struct
{
	thinker_t thinker;   ///< Thinker structure for scrolling.
	fixed_t dx, dy;      ///< (dx,dy) scroll speeds.
	INT32 affectee;      ///< Number of affected sidedef or sector.
	INT32 control;       ///< Control sector (-1 if none) used to control scrolling.
	fixed_t last_height; ///< Last known height of control sector.
	fixed_t vdx, vdy;    ///< Accumulated velocity if accelerative.
	INT32 accel;         ///< Whether it's accelerative.
	INT32 exclusive;     ///< If a conveyor, same property as in pusher_t
	/** Types of generalized scrollers.
	*/
	enum
	{
		sc_side,         ///< Scroll wall texture on a sidedef.
		sc_floor,        ///< Scroll floor.
		sc_ceiling,      ///< Scroll ceiling.
		sc_carry,        ///< Carry objects on floor.
		sc_carry_ceiling,///< Carry objects on ceiling (for 3Dfloor conveyors).
	} type;
} scroll_t;

void T_Scroll(scroll_t *s);
void T_LaserFlash(laserthink_t *flash);

/** Friction for ice/sludge effects.
  */
typedef struct
{
	thinker_t thinker;   ///< Thinker structure for friction.
	INT32 friction;      ///< Friction value, 0xe800 = normal.
	INT32 movefactor;    ///< Inertia factor when adding to momentum, FRACUNIT = normal.
	INT32 affectee;      ///< Number of affected sector.
	INT32 referrer;      ///< If roverfriction == true, then this will contain the sector # of the control sector where the effect was applied.
	UINT8 roverfriction;  ///< flag for whether friction originated from a FOF or not
} friction_t;

// Friction defines.
#define ORIG_FRICTION          (0xE8 << (FRACBITS-8)) ///< Original value.
#define ORIG_FRICTION_FACTOR   (8 << (FRACBITS-8))    ///< Original value.

void T_Friction(friction_t *f);

typedef enum
{
	p_push,        ///< Point pusher or puller.
	p_wind,        ///< Wind.
	p_current,     ///< Current.
	p_upcurrent,   ///< Upwards current.
	p_downcurrent, ///< Downwards current.
	p_upwind,      ///< Upwards wind.
	p_downwind     ///< Downwards wind.
} pushertype_e;

// Model for pushers for push/pull effects
typedef struct
{
	thinker_t thinker; ///< Thinker structure for push/pull effect.
	/** Types of push/pull effects.
	*/
	pushertype_e type;  ///< Type of push/pull effect.
	mobj_t *source;     ///< Point source if point pusher/puller.
	INT32 x_mag;        ///< X strength.
	INT32 y_mag;        ///< Y strength.
	INT32 magnitude;    ///< Vector strength for point pusher/puller.
	INT32 radius;       ///< Effective radius for point pusher/puller.
	INT32 x, y, z;      ///< Point source if point pusher/puller.
	INT32 affectee;     ///< Number of affected sector.
	UINT8 roverpusher;   ///< flag for whether pusher originated from a FOF or not
	INT32 referrer;     ///< If roverpusher == true, then this will contain the sector # of the control sector where the effect was applied.
	INT32 exclusive;    /// < Once this affect has been applied to a mobj, no other pushers may affect it.
	INT32 slider;       /// < Should the player go into an uncontrollable slide?
} pusher_t;

// Model for disappearing/reappearing FOFs
typedef struct
{
	thinker_t thinker;  ///< Thinker structure for effect.
	tic_t appeartime;   ///< Tics to be appeared for
	tic_t disappeartime;///< Tics to be disappeared for
	tic_t offset;       ///< Time to wait until thinker starts
	tic_t timer;        ///< Timer between states
	INT32 affectee;     ///< Number of affected line
	INT32 sourceline;   ///< Number of source line
	INT32 exists;       ///< Exists toggle
} disappear_t;

void T_Disappear(disappear_t *d);

// Model for fading FOFs
typedef struct
{
	thinker_t thinker;  ///< Thinker structure for effect.
	ffloor_t *rover;    ///< Target ffloor
	extracolormap_t *dest_exc; ///< Colormap to fade to
	UINT32 sectornum;    ///< Number of ffloor target sector
	UINT32 ffloornum;    ///< Number of ffloor of target sector
	INT32 alpha;        ///< Internal alpha counter
	INT16 sourcevalue;  ///< Transparency value to fade from
	INT16 destvalue;    ///< Transparency value to fade to
	INT16 destlightlevel; ///< Light level to fade to
	INT16 speed;        ///< Speed to fade by
	boolean ticbased;    ///< Tic-based logic toggle
	INT32 timer;        ///< Timer for tic-based logic
	boolean doexists;   ///< Handle FF_EXISTS
	boolean dotranslucent; ///< Handle FF_TRANSLUCENT
	boolean dolighting; ///< Handle shadows and light blocks
	boolean docolormap; ///< Handle colormaps
	boolean docollision; ///< Handle interactive flags
	boolean doghostfade; ///< No interactive flags during fading
	boolean exactalpha; ///< Use exact alpha values (opengl)
} fade_t;

void T_Fade(fade_t *d);

// Model for fading colormaps

typedef struct
{
	thinker_t thinker;          ///< Thinker structure for effect.
	sector_t *sector;           ///< Sector where action is taking place.
	extracolormap_t *source_exc;
	extracolormap_t *dest_exc;
	boolean ticbased;           ///< Tic-based timing
	INT32 duration;             ///< Total duration for tic-based logic (OR: speed increment)
	INT32 timer;                ///< Timer for tic-based logic (OR: internal speed counter)
} fadecolormap_t;

void T_FadeColormap(fadecolormap_t *d);

// Prototype functions for pushers
void T_Pusher(pusher_t *p);
mobj_t *P_GetPushThing(UINT32 s);

// Plane displacement
typedef struct
{
	thinker_t thinker;   ///< Thinker structure for plane displacement effect.
	INT32 affectee;      ///< Number of affected sector.
	INT32 control;       ///< Control sector used to control plane positions.
	fixed_t last_height; ///< Last known height of control sector.
	fixed_t speed;       ///< Plane movement speed.
	UINT8 reverse;       ///< Move in reverse direction to control sector?
	/** Types of plane displacement effects.
	*/
	enum
	{
		pd_floor,        ///< Displace floor.
		pd_ceiling,      ///< Displace ceiling.
		pd_both,         ///< Displace both floor AND ceiling.
	} type;
} planedisplace_t;

void T_PlaneDisplace(planedisplace_t *pd);

void P_CalcHeight(player_t *player);

sector_t *P_ThingOnSpecial3DFloor(mobj_t *mo);

#endif
