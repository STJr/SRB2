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

// Amount (dx, dy) vector linedef is shifted right to get scroll amount
#define SCROLL_SHIFT 5

typedef enum
{
	TMM_DOUBLESIZE      = 1,
	TMM_SILENT          = 1<<1,
	TMM_ALLOWYAWCONTROL = 1<<2,
	TMM_SWING           = 1<<3,
	TMM_MACELINKS       = 1<<4,
	TMM_CENTERLINK      = 1<<5,
	TMM_CLIP            = 1<<6,
	TMM_ALWAYSTHINK     = 1<<7,
} textmapmaceflags_t;

typedef enum
{
	TMDA_BOTTOMOFFSET = 1,
	TMDA_BOTTOM       = 1<<1,
	TMDA_MIDDLE       = 1<<2,
	TMDA_TOP          = 1<<3,
} textmapdronealignment_t;

typedef enum
{
	TMSF_RETRACTED  = 1,
	TMSF_INTANGIBLE = 1<<1,
} textmapspikeflags_t;

typedef enum
{
	TMFF_AIMLESS    = 1,
	TMFF_STATIONARY = 1<<1,
	TMFF_HOP        = 1<<2,
} textmapflickyflags_t;

typedef enum
{
	TMFH_NOFLAME = 1,
	TMFH_CORONA  = 1<<1,
} textmapflameholderflags_t;

typedef enum
{
	TMDS_NOGRAVITY   = 1,
	TMDS_ROTATEEXTRA = 1<<1,
} textmapdiagonalspringflags_t;

typedef enum
{
	TMF_INVISIBLE       = 1,
	TMF_NODISTANCECHECK = 1<<1,
} textmapfanflags_t;

typedef enum
{
	TMGD_BACK  = 0,
	TMGD_RIGHT = 1,
	TMGD_LEFT  = 2,
} textmapguarddirection_t;

typedef enum
{
	TMNI_BONUSONLY = 1,
	TMNI_REVEAL    = 1<<1,
} textmapnightsitem_t;

typedef enum
{
	TMP_NORMAL    = 0,
	TMP_SLIDE     = 1,
	TMP_IMMOVABLE = 2,
	TMP_CLASSIC   = 3,
} textmappushabletype_t;

typedef enum
{
	TMED_NONE  = 0,
	TMED_RIGHT = 1,
	TMED_LEFT  = 2,
} textmapeggrobodirection_t;

typedef enum
{
	TMMR_SAME   = 0,
	TMMR_WEAK   = 1,
	TMMR_STRONG = 2,
} textmapmonitorrespawn_t;

typedef enum
{
	TMF_GRAYSCALE = 1,
	TMF_SKIPINTRO = 1<<1,
} textmapfangflags_t;

typedef enum
{
	TMB_NODEATHFLING = 1,
	TMB_BARRIER      = 1<<1,
} textmapbrakflags_t;

typedef enum
{
	TMEF_SKIPTALLY    = 1,
	TMEF_EMERALDCHECK = 1<<1,
} textmapexitflags_t;

typedef enum
{
	TMSP_NOTELEPORT = 1,
	TMSP_FORCESPIN  = 1<<1,
} textmapspeedpadflags_t;

//FOF flags
typedef enum
{
	TMFA_NOPLANES    = 1,
	TMFA_NOSIDES     = 1<<1,
	TMFA_INSIDES     = 1<<2,
	TMFA_ONLYINSIDES = 1<<3,
	TMFA_NOSHADE     = 1<<4,
	TMFA_SPLAT       = 1<<5,
} textmapfofappearance_t;

typedef enum
{
	TMFT_INTANGIBLETOP    = 1,
	TMFT_INTANGIBLEBOTTOM = 1<<1,
	TMFT_DONTBLOCKPLAYER  = 1<<2,
	TMFT_VISIBLEFROMINSIDE = (TMFT_INTANGIBLETOP|TMFT_INTANGIBLEBOTTOM|TMFT_DONTBLOCKPLAYER),
	TMFT_DONTBLOCKOTHERS  = 1<<3,
	TMFT_INTANGIBLE       = (TMFT_DONTBLOCKPLAYER|TMFT_DONTBLOCKOTHERS),
} textmapfoftangibility_t;

typedef enum
{
	TMFW_NOSIDES      = 1,
	TMFW_DOUBLESHADOW = 1<<1,
	TMFW_COLORMAPONLY = 1<<2,
	TMFW_NORIPPLE     = 1<<3,
	TMFW_GOOWATER     = 1<<4,
	TMFW_SPLAT        = 1<<5,
} textmapfofwater_t;

typedef enum
{
	TMFB_REVERSE  = 1,
	TMFB_SPINDASH = 1<<1,
	TMFB_DYNAMIC  = 1<<2,
} textmapfofbobbing_t;

typedef enum
{
	TMFC_NOSHADE     = 1,
	TMFC_NORETURN    = 1<<1,
	TMFC_AIRBOB      = 1<<2,
	TMFC_FLOATBOB    = 1<<3,
	TMFC_SPLAT       = 1<<4,
} textmapfofcrumbling_t;

typedef enum
{
	TMFR_REVERSE  = 1,
	TMFR_SPINDASH = 1<<1,
} textmapfofrising_t;

typedef enum
{
	TMFM_BRICK     = 1,
	TMFM_INVISIBLE = 1<<1,
} textmapfofmario_t;

typedef enum
{
	TMFB_TOUCH,
	TMFB_SPIN,
	TMFB_REGULAR,
	TMFB_STRONG,
} textmapfofbusttype_t;

typedef enum
{
	TMFB_PUSHABLES   = 1,
	TMFB_EXECUTOR    = 1<<1,
	TMFB_ONLYBOTTOM  = 1<<2,
	TMFB_SPLAT       = 1<<3,
} textmapfofbustflags_t;

typedef enum
{
	TMFL_NOBOSSES = 1,
	TMFL_SPLAT    = 1<<1,
} textmapfoflaserflags_t;

typedef enum
{
	TMT_CONTINUOUS           = 0,
	TMT_ONCE                 = 1,
	TMT_EACHTIMEMASK         = TMT_ONCE,
	TMT_EACHTIMEENTER        = 2,
	TMT_EACHTIMEENTERANDEXIT = 3,
} textmaptriggertype_t;

typedef enum
{
	TMXT_CONTINUOUS           = 0,
	TMXT_EACHTIMEMASK         = TMXT_CONTINUOUS,
	TMXT_EACHTIMEENTER        = 1,
	TMXT_EACHTIMEENTERANDEXIT = 2,
} textmapxtriggertype_t;

typedef enum
{
	TMF_HASALL        = 0,
	TMF_HASANY        = 1,
	TMF_HASEXACTLY    = 2,
	TMF_DOESNTHAVEALL = 3,
	TMF_DOESNTHAVEANY = 4,
} textmapflagcheck_t;

typedef enum
{
	TMT_RED  = 0,
	TMT_BLUE = 1,
} textmapteam_t;

typedef enum
{
	TMC_EQUAL = 0,
	TMC_LTE   = 1,
	TMC_GTE   = 2,
} textmapcomparison_t;

typedef enum
{
	TMG_NORMAL  = 0,
	TMG_REVERSE = 1,
	TMG_TEMPREVERSE = 2,
} textmapgravity_t;

typedef enum
{
	TMNP_FASTEST   = 0,
	TMNP_SLOWEST   = 1,
	TMNP_TRIGGERER = 2,
} textmapnightsplayer_t;

typedef enum
{
	TMN_ALWAYS       = 0,
	TMN_FROMNONIGHTS = 1,
	TMN_FROMNIGHTS   = 2,
} textmapnighterizeoptions_t;

typedef enum
{
	TMN_BONUSLAPS       = 1,
	TMN_LEVELCOMPLETION = 1<<2,
} textmapnightserizeflags_t;

typedef enum
{
	TMD_ALWAYS         = 0,
	TMD_NOBODYNIGHTS   = 1,
	TMD_SOMEBODYNIGHTS = 2,
} textmapdenighterizeoptions_t;

typedef enum
{
	TMS_IFENOUGH    = 0,
	TMS_IFNOTENOUGH = 1,
	TMS_ALWAYS      = 2,
} textmapspherescheck_t;

typedef enum
{
	TMI_BONUSLAPS = 1,
	TMI_ENTER     = 1<<2,
} textmapideyacaptureflags_t;

typedef enum
{
	TMP_FLOOR = 0,
	TMP_CEILING = 1,
	TMP_BOTH = 2,
} textmapplanes_t;

typedef enum
{
	TMT_ADD          = 0,
	TMT_REMOVE       = 1,
	TMT_REPLACEFIRST = 2,
	TMT_TRIGGERTAG   = 3,
} textmaptagoptions_t;

typedef enum
{
	TMT_SILENT       = 1,
	TMT_KEEPANGLE    = 1<<1,
	TMT_KEEPMOMENTUM = 1<<2,
	TMT_RELATIVE     = 1<<3,
} textmapteleportflags_t;

typedef enum
{
	TMM_ALLPLAYERS = 1,
	TMM_OFFSET = 1<<1,
	TMM_FADE = 1<<2,
	TMM_NORELOAD = 1<<3,
	TMM_FORCERESET = 1<<4,
	TMM_NOLOOP = 1<<5,
} textmapmusicflags_t;

typedef enum
{
	TMSS_TRIGGERMOBJ   = 0,
	TMSS_TRIGGERSECTOR = 1,
	TMSS_NOWHERE       = 2,
	TMSS_TAGGEDSECTOR  = 3,
} textmapsoundsource_t;

typedef enum
{
	TMSL_EVERYONE     = 0,
	TMSL_TRIGGERER    = 1,
	TMSL_TAGGEDSECTOR = 2,
} textmapsoundlistener_t;

typedef enum
{
	TML_SECTOR  = 0,
	TML_FLOOR   = 1,
	TML_CEILING = 2,
} textmaplightareas_t;

typedef enum
{
	TMLC_NOSECTOR  = 1,
	TMLC_NOFLOOR   = 1<<1,
	TMLC_NOCEILING = 1<<2,
} textmaplightcopyflags_t;

typedef enum
{
	TMF_RELATIVE = 1,
	TMF_OVERRIDE = 1<<1,
	TMF_TICBASED = 1<<2,
} textmapfadeflags_t;

typedef enum
{
	TMB_USETARGET = 1,
	TMB_SYNC      = 1<<1,
} textmapblinkinglightflags_t;

typedef enum
{
	TMFR_NORETURN  = 1,
	TMFR_CHECKFLAG = 1<<1,
} textmapfofrespawnflags_t;

typedef enum
{
	TMST_RELATIVE          = 1,
	TMST_DONTDOTRANSLUCENT = 1<<1,
} textmapsettranslucencyflags_t;

typedef enum
{
	TMFT_RELATIVE          = 1,
	TMFT_OVERRIDE          = 1<<1,
	TMFT_TICBASED          = 1<<2,
	TMFT_IGNORECOLLISION   = 1<<3,
	TMFT_GHOSTFADE         = 1<<4,
	TMFT_DONTDOTRANSLUCENT = 1<<5,
	TMFT_DONTDOEXISTS      = 1<<6,
	TMFT_DONTDOLIGHTING    = 1<<7,
	TMFT_DONTDOCOLORMAP    = 1<<8,
	TMFT_USEEXACTALPHA     = 1<<9,
} textmapfadetranslucencyflags_t;

typedef enum
{
	TMS_VIEWPOINT   = 0,
	TMS_CENTERPOINT = 1,
	TMS_BOTH        = 2,
} textmapskybox_t;

typedef enum
{
	TMP_CLOSE          = 1,
	TMP_RUNPOSTEXEC    = 1<<1,
	TMP_CALLBYNAME     = 1<<2,
	TMP_KEEPCONTROLS   = 1<<3,
	TMP_KEEPREALTIME   = 1<<4,
	//TMP_ALLPLAYERS     = 1<<5,
	//TMP_FREEZETHINKERS = 1<<6,
} textmappromptflags_t;

typedef enum
{
	TMF_NOCHANGE = 0,
	TMF_ADD      = 1,
	TMF_REMOVE   = 2,
} textmapsetflagflags_t;

typedef enum
{
	TMSD_FRONT = 0,
	TMSD_BACK = 1,
	TMSD_FRONTBACK = 2,
} textmapsides_t;

typedef enum
{
	TMS_SCROLLCARRY = 0,
	TMS_SCROLLONLY = 1,
	TMS_CARRYONLY = 2,
} textmapscroll_t;

typedef enum
{
	TMST_REGULAR = 0,
	TMST_ACCELERATIVE = 1,
	TMST_DISPLACEMENT = 2,
	TMST_TYPEMASK = 3,
	TMST_NONEXCLUSIVE = 4,
} textmapscrolltype_t;

typedef enum
{
	TMPF_SLIDE = 1,
	TMPF_NONEXCLUSIVE = 1<<1,
} textmappusherflags_t;

typedef enum
{
	TMPP_NOZFADE      = 1,
	TMPP_PUSHZ        = 1<<1,
	TMPP_NONEXCLUSIVE = 1<<2,
} textmappointpushflags_t;

typedef enum
{
	TMB_TRANSLUCENT     = 0,
	TMB_ADD             = 1,
	TMB_SUBTRACT        = 2,
	TMB_REVERSESUBTRACT = 3,
	TMB_MODULATE        = 4,
} textmapblendmodes_t;

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
void P_ApplyFlatAlignment(sector_t* sector, angle_t flatangle, fixed_t xoffs, fixed_t yoffs, boolean floor, boolean ceiling);
fixed_t P_GetSectorGravityFactor(sector_t *sec);
void P_SpawnSpecials(boolean fromnetsave);

// every tic
void P_UpdateSpecials(void);
sector_t *P_MobjTouchingSectorSpecial(mobj_t *mo, INT32 section, INT32 number);
sector_t *P_ThingOnSpecial3DFloor(mobj_t *mo);
sector_t *P_MobjTouchingSectorSpecialFlag(mobj_t *mo, sectorspecialflags_t flag);
sector_t *P_PlayerTouchingSectorSpecial(player_t *player, INT32 section, INT32 number);
sector_t *P_PlayerTouchingSectorSpecialFlag(player_t *player, sectorspecialflags_t flag);
void P_PlayerInSpecialSector(player_t *player);
void P_CheckMobjTrigger(mobj_t *mobj, boolean pushable);
sector_t *P_FindPlayerTrigger(player_t *player, line_t *sourceline);
boolean P_IsPlayerValid(size_t playernum);
boolean P_CanPlayerTrigger(size_t playernum);
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

boolean P_IsMobjTouchingSectorPlane(mobj_t *mo, sector_t *sec);
boolean P_IsMobjTouching3DFloor(mobj_t *mo, ffloor_t *ffloor, sector_t *sec);
boolean P_IsMobjTouchingPolyobj(mobj_t *mo, polyobj_t *po, sector_t *polysec);

void P_SwitchWeather(INT32 weathernum);

boolean P_RunTriggerLinedef(line_t *triggerline, mobj_t *actor, sector_t *caller);
void P_LinedefExecute(INT16 tag, mobj_t *actor, sector_t *caller);
void P_RunNightserizeExecutors(mobj_t *actor);
void P_RunDeNightserizeExecutors(mobj_t *actor);
void P_RunNightsLapExecutors(mobj_t *actor);
void P_RunNightsCapsuleTouchExecutors(mobj_t *actor, boolean entering, boolean enoughspheres);

UINT16 P_GetFFloorID(ffloor_t *fflr);
ffloor_t *P_GetFFloorByID(sector_t *sec, UINT16 id);

// Use this when you don't know the type of your thinker data struct but need to access its thinker.
typedef struct
{
	thinker_t thinker;
} thinkerdata_t;

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
	INT16 maxlight;    ///< The brightest light level to use.
	INT16 minlight;    ///< The darkest light level to use.
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
	INT16 minlight;    ///< The minimum light level to use.
	INT16 maxlight;    ///< The maximum light level to use.
	INT32 darktime;    ///< How INT32 to use minlight.
	INT32 brighttime;  ///< How INT32 to use maxlight.
} strobe_t;

typedef struct
{
	thinker_t thinker;
	sector_t *sector;
	INT16 minlight;
	INT16 maxlight;
	INT16 direction;
	INT16 speed;
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
fireflicker_t *P_SpawnAdjustableFireFlicker(sector_t *sector, INT16 lighta, INT16 lightb, INT32 length);
void T_LightningFlash(lightflash_t *flash);
void T_StrobeFlash(strobe_t *flash);

void P_SpawnLightningFlash(sector_t *sector);
strobe_t * P_SpawnAdjustableStrobeFlash(sector_t *sector, INT16 lighta, INT16 lightb, INT32 darktime, INT32 brighttime, boolean inSync);

void T_Glow(glow_t *g);
glow_t *P_SpawnAdjustableGlowingLight(sector_t *sector, INT16 lighta, INT16 lightb, INT32 length);

void P_FadeLightBySector(sector_t *sector, INT32 destvalue, INT32 speed, boolean ticbased);
void P_FadeLight(INT16 tag, INT32 destvalue, INT32 speed, boolean ticbased, boolean force, boolean relative);
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
	lowerToLowestFast,

	instantRaise, // instant-move for ceilings

	crushAndRaise,
	raiseAndCrush,
	crushCeilOnce,
	crushBothOnce,

	moveCeilingByFrontSector,
	instantMoveCeilingByFrontSector,

	moveCeilingByDistance,

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
	fixed_t delay;
	fixed_t delaytimer;
	UINT8 crush;           ///< Whether to crush things or not.

	INT32 texture;        ///< The number of a flat to use when done.
	INT32 direction;      ///< 1 = up, 0 = waiting, -1 = down.

	// ID
	INT16 tag;            ///< Tag of linedef executor to run when movement is done.
	fixed_t origspeed;    ///< The original, "real" speed.
	INT32 sourceline;     ///< Index of the source linedef
} ceiling_t;

#define CEILSPEED (FRACUNIT)

INT32 EV_DoCeiling(mtag_t tag, line_t *line, ceiling_e type);

INT32 EV_DoCrush(mtag_t tag, line_t *line, ceiling_e type);
void T_CrushCeiling(ceiling_t *ceiling);

void T_MoveCeiling(ceiling_t *ceiling);

//
// P_FLOOR
//
typedef enum
{
	// raise floor to next highest surrounding floor
	raiseFloorToNearestFast,

	// move the floor down instantly
	instantLower,

	moveFloorByFrontSector,
	instantMoveFloorByFrontSector,

	moveFloorByDistance,

	bounceFloor,
	bounceFloorCrush,

	crushFloorOnce,
} floor_e;

typedef enum
{
	elevateUp,
	elevateDown,
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
	INT16 tag;
	INT32 sourceline;
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
void EV_DoFloor(mtag_t tag, line_t *line, floor_e floortype);
void EV_DoElevator(mtag_t tag, line_t *line, elevator_e elevtype);
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
	p_wind,        ///< Wind.
	p_current,     ///< Current.
} pushertype_e;

// Model for pushers for push/pull effects
typedef struct
{
	thinker_t thinker;  ///< Thinker structure for pusher effect.
	pushertype_e type;  ///< Type of pusher effect.
	fixed_t x_mag;      ///< X strength.
	fixed_t y_mag;      ///< Y strength.
	fixed_t z_mag;      ///< Z strength.
	INT32 affectee;     ///< Number of affected sector.
	UINT8 roverpusher;  ///< flag for whether pusher originated from a FOF or not
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
	boolean doexists;   ///< Handle FOF_EXISTS
	boolean dotranslucent; ///< Handle FOF_TRANSLUCENT
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

// Prototype function for pushers
void T_Pusher(pusher_t *p);

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

#endif
