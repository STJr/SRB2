// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_local.h
/// \brief Play functions, animation, global header

#ifndef __P_LOCAL__
#define __P_LOCAL__

#include "command.h"
#include "d_player.h"
#include "d_think.h"
#include "m_fixed.h"
#include "m_bbox.h"
#include "p_tick.h"
#include "r_defs.h"
#include "p_maputl.h"

#define FLOATSPEED (FRACUNIT*4)

#define VIEWHEIGHTS "41"

// Maximum player score.
#define MAXSCORE 999999990

// mapblocks are used to check movement
// against lines and things
#define MAPBLOCKUNITS 128
#define MAPBLOCKSIZE  (MAPBLOCKUNITS*FRACUNIT)
#define MAPBLOCKSHIFT (FRACBITS+7)
#define MAPBMASK      (MAPBLOCKSIZE-1)
#define MAPBTOFRAC    (MAPBLOCKSHIFT-FRACBITS)

// Convenience macro to fix issue with collision along bottom/left edges of blockmap -Red
#define BMBOUNDFIX(xl, xh, yl, yh) {if (xl > xh) xl = 0; if (yl > yh) yl = 0;}

// player radius used only in am_map.c
#define PLAYERRADIUS (16*FRACUNIT)

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger,
// but we do not have any moving sectors nearby
#define MAXRADIUS (32*FRACUNIT)

// max Z move up or down without jumping
// above this, a height difference is considered as a 'dropoff'
#define MAXSTEPMOVE (24*FRACUNIT)

#ifdef ESLOPE
// [RH] Minimum floorplane.c value for walking
// The lower the value, the steeper the slope is
#define SECPLANESTEEPSLOPE		46000
// ESLOPE stuff - a slope of 4 or lower is so level, treat it as flat
#define LEVELSLOPE 4
#define STEEPSLOPE 65
#endif

#define USERANGE (64*FRACUNIT)
#define MELEERANGE (64*FRACUNIT)
#define MISSILERANGE (32*64*FRACUNIT)

#define AIMINGTOSLOPE(aiming) FINESINE((aiming>>ANGLETOFINESHIFT) & FINEMASK)

#define mariomode (maptol & TOL_MARIO)
#define twodlevel (maptol & TOL_2D)

//
// P_TICK
//

// both the head and tail of the thinker list
extern thinker_t thinkercap;
extern INT32 runcount;

void P_InitThinkers(void);
void P_AddThinker(thinker_t *thinker);
void P_RemoveThinker(thinker_t *thinker);

//
// P_USER
//
typedef struct camera_s
{
	boolean chase;
	angle_t aiming;

	// Things used by FS cameras.
	fixed_t viewheight;
	angle_t startangle;

	// Camera demobjerization
	// Info for drawing: position.
	fixed_t x, y, z;

	//More drawing info: to determine current sprite.
	angle_t angle; // orientation

	struct subsector_s *subsector;

	// The closest interval over all contacted Sectors (or Things).
	fixed_t floorz;
	fixed_t ceilingz;

	// For movement checking.
	fixed_t radius;
	fixed_t height;

	fixed_t relativex;

	// Momentums, used to update position.
	fixed_t momx, momy, momz;
} camera_t;

extern camera_t camera, camera2;
extern consvar_t cv_cam_dist, cv_cam_still, cv_cam_height;
extern consvar_t cv_cam_speed, cv_cam_rotate, cv_cam_rotspeed;

extern consvar_t cv_cam2_dist, cv_cam2_still, cv_cam2_height;
extern consvar_t cv_cam2_speed, cv_cam2_rotate, cv_cam2_rotspeed;

extern fixed_t t_cam_dist, t_cam_height, t_cam_rotate;
extern fixed_t t_cam2_dist, t_cam2_height, t_cam2_rotate;

fixed_t P_GetPlayerHeight(player_t *player);
fixed_t P_GetPlayerSpinHeight(player_t *player);
INT32 P_GetPlayerControlDirection(player_t *player);
void P_AddPlayerScore(player_t *player, UINT32 amount);
void P_ResetCamera(player_t *player, camera_t *thiscam);
boolean P_TryCameraMove(fixed_t x, fixed_t y, camera_t *thiscam);
void P_SlideCameraMove(camera_t *thiscam);
boolean P_MoveChaseCamera(player_t *player, camera_t *thiscam, boolean resetcalled);
boolean P_PlayerInPain(player_t *player);
void P_DoPlayerPain(player_t *player, mobj_t *source, mobj_t *inflictor);
void P_ResetPlayer(player_t *player);
boolean P_IsLocalPlayer(player_t *player);

boolean P_IsObjectInGoop(mobj_t *mo);
boolean P_IsObjectOnGround(mobj_t *mo);
boolean P_IsObjectOnGroundIn(mobj_t *mo, sector_t *sec);
boolean P_InSpaceSector(mobj_t *mo);
boolean P_InQuicksand(mobj_t *mo);

#ifdef ESLOPE
boolean P_IsObjectOnSlope(mobj_t *mo, boolean ceiling);
boolean P_SlopeGreaterThan(mobj_t *mo, boolean ceiling, int value);
boolean P_SlopeLessThan(mobj_t *mo, boolean ceiling, int value);
#endif

void P_SetObjectMomZ(mobj_t *mo, fixed_t value, boolean relative);
void P_RestoreMusic(player_t *player);
void P_SpawnShieldOrb(player_t *player);
mobj_t *P_SpawnGhostMobj(mobj_t *mobj);
void P_GivePlayerRings(player_t *player, INT32 num_rings);
void P_GivePlayerLives(player_t *player, INT32 numlives);
UINT8 P_GetNextEmerald(void);
void P_GiveEmerald(boolean spawnObj);
void P_ResetScore(player_t *player);
boolean P_AutoPause(void);

void P_DoJumpShield(player_t *player);
void P_BlackOw(player_t *player);
void P_ElementalFireTrail(player_t *player);

void P_DoPityCheck(player_t *player);
void P_PlayerThink(player_t *player);
void P_PlayerAfterThink(player_t *player);
void P_DoPlayerExit(player_t *player);
void P_NightserizePlayer(player_t *player, INT32 ptime);

void P_InstaThrust(mobj_t *mo, angle_t angle, fixed_t move);
fixed_t P_ReturnThrustX(mobj_t *mo, angle_t angle, fixed_t move);
fixed_t P_ReturnThrustY(mobj_t *mo, angle_t angle, fixed_t move);
void P_InstaThrustEvenIn2D(mobj_t *mo, angle_t angle, fixed_t move);

boolean P_LookForEnemies(player_t *player);
void P_NukeEnemies(mobj_t *inflictor, mobj_t *source, fixed_t radius);
void P_HomingAttack(mobj_t *source, mobj_t *enemy); /// \todo doesn't belong in p_user
boolean P_SuperReady(player_t *player);
void P_DoJump(player_t *player, boolean soundandstate);
boolean P_TransferToNextMare(player_t *player);
UINT8 P_FindLowestMare(void);
void P_FindEmerald(void);
void P_TransferToAxis(player_t *player, INT32 axisnum);
boolean P_PlayerMoving(INT32 pnum);
void P_SpawnThokMobj(player_t *player);
void P_SpawnSpinMobj(player_t *player, mobjtype_t type);
void P_Telekinesis(player_t *player, fixed_t thrust, fixed_t range);

void P_PlayLivesJingle(player_t *player);
#define P_PlayRinglossSound(s)	S_StartSound(s, (mariomode) ? sfx_mario8 : sfx_altow1 + P_RandomKey(4));
#define P_PlayDeathSound(s)		S_StartSound(s, sfx_altdi1 + P_RandomKey(4));
#define P_PlayVictorySound(s)	S_StartSound(s, sfx_victr1 + P_RandomKey(4));


//
// P_MOBJ
//
#define ONFLOORZ INT32_MIN
#define ONCEILINGZ INT32_MAX

// Time interval for item respawning.
// WARNING MUST be a power of 2
#define ITEMQUESIZE 1024

extern mapthing_t *itemrespawnque[ITEMQUESIZE];
extern tic_t itemrespawntime[ITEMQUESIZE];
extern size_t iquehead, iquetail;
extern consvar_t cv_gravity, cv_viewheight;

void P_RespawnSpecials(void);

mobj_t *P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type);

void P_RecalcPrecipInSector(sector_t *sector);
void P_PrecipitationEffects(void);

void P_RemoveMobj(mobj_t *th);
boolean P_MobjWasRemoved(mobj_t *th);
void P_RemoveSavegameMobj(mobj_t *th);
boolean P_SetPlayerMobjState(mobj_t *mobj, statenum_t state);
boolean P_SetMobjState(mobj_t *mobj, statenum_t state);
void P_RunShields(void);
void P_MobjThinker(mobj_t *mobj);
boolean P_RailThinker(mobj_t *mobj);
void P_PushableThinker(mobj_t *mobj);
void P_SceneryThinker(mobj_t *mobj);

fixed_t P_GetFloorZ(mobj_t *mobj, sector_t *sector, fixed_t x, fixed_t y, line_t *line);
fixed_t P_GetCeilingZ(mobj_t *mobj, sector_t *sector, fixed_t x, fixed_t y, line_t *line);
fixed_t P_GetFOFTopZ(mobj_t *mobj, sector_t *sector, ffloor_t *fof, fixed_t x, fixed_t y, line_t *line);
fixed_t P_GetFOFBottomZ(mobj_t *mobj, sector_t *sector, ffloor_t *fof, fixed_t x, fixed_t y, line_t *line);

boolean P_InsideANonSolidFFloor(mobj_t *mobj, ffloor_t *rover);
boolean P_CheckDeathPitCollide(mobj_t *mo);
boolean P_CheckSolidLava(mobj_t *mo, ffloor_t *rover);

mobj_t *P_SpawnMissile(mobj_t *source, mobj_t *dest, mobjtype_t type);
mobj_t *P_SpawnXYZMissile(mobj_t *source, mobj_t *dest, mobjtype_t type, fixed_t x, fixed_t y, fixed_t z);
mobj_t *P_SpawnPointMissile(mobj_t *source, fixed_t xa, fixed_t ya, fixed_t za, mobjtype_t type, fixed_t x, fixed_t y, fixed_t z);
mobj_t *P_SpawnAlteredDirectionMissile(mobj_t *source, mobjtype_t type, fixed_t x, fixed_t y, fixed_t z, INT32 shiftingAngle);
mobj_t *P_SPMAngle(mobj_t *source, mobjtype_t type, angle_t angle, UINT8 aimtype, UINT32 flags2);
#define P_SpawnPlayerMissile(s,t,f) P_SPMAngle(s,t,s->angle,true,f)
#ifdef SEENAMES
#define P_SpawnNameFinder(s,t) P_SPMAngle(s,t,s->angle,true,0)
#endif
void P_ColorTeamMissile(mobj_t *missile, player_t *source);
SINT8 P_MobjFlip(mobj_t *mobj);
boolean P_WeaponOrPanel(mobjtype_t type);

boolean P_CameraThinker(player_t *player, camera_t *thiscam, boolean resetcalled);

void P_Attract(mobj_t *source, mobj_t *enemy, boolean nightsgrab);
mobj_t *P_GetClosestAxis(mobj_t *source);

boolean P_CanRunOnWater(player_t *player, ffloor_t *rover);

void P_FlashPal(player_t *pl, UINT16 type, UINT16 duration);
#define PAL_WHITE    1
#define PAL_MIXUP    2
#define PAL_RECYCLE  3
#define PAL_NUKE     4

//
// P_ENEMY
//

// main player in game
extern player_t *stplyr; // for splitscreen correct palette changes and overlay

// Is there a better place for these?
extern INT32 var1;
extern INT32 var2;

boolean P_CheckMeleeRange(mobj_t *actor);
boolean P_JetbCheckMeleeRange(mobj_t *actor);
boolean P_FaceStabCheckMeleeRange(mobj_t *actor);
boolean P_SkimCheckMeleeRange(mobj_t *actor);
boolean P_CheckMissileRange(mobj_t *actor);

void P_NewChaseDir(mobj_t *actor);
boolean P_LookForPlayers(mobj_t *actor, boolean allaround, boolean tracer, fixed_t dist);

//
// P_MAP
//

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
extern boolean floatok;
extern fixed_t tmfloorz;
extern fixed_t tmceilingz;
extern mobj_t *tmfloorthing, *tmthing;
extern camera_t *mapcampointer;
extern fixed_t tmx;
extern fixed_t tmy;
#ifdef ESLOPE
extern pslope_t *tmfloorslope, *tmceilingslope;
#endif

/* cphipps 2004/08/30 */
extern void P_MapStart(void);
extern void P_MapEnd(void);

extern line_t *ceilingline;
extern line_t *blockingline;
extern msecnode_t *sector_list;

extern mprecipsecnode_t *precipsector_list;

void P_UnsetThingPosition(mobj_t *thing);
void P_SetThingPosition(mobj_t *thing);
void P_SetUnderlayPosition(mobj_t *thing);

boolean P_CheckPosition(mobj_t *thing, fixed_t x, fixed_t y);
boolean P_CheckCameraPosition(fixed_t x, fixed_t y, camera_t *thiscam);
boolean P_TryMove(mobj_t *thing, fixed_t x, fixed_t y, boolean allowdropoff);
boolean P_Move(mobj_t *actor, fixed_t speed);
boolean P_TeleportMove(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z);
void P_SlideMove(mobj_t *mo);
void P_BounceMove(mobj_t *mo);
boolean P_CheckSight(mobj_t *t1, mobj_t *t2);
void P_CheckHoopPosition(mobj_t *hoopthing, fixed_t x, fixed_t y, fixed_t z, fixed_t radius);

boolean P_CheckSector(sector_t *sector, boolean crunch);

void P_DelSeclist(msecnode_t *node);
void P_DelPrecipSeclist(mprecipsecnode_t *node);

void P_CreateSecNodeList(mobj_t *thing, fixed_t x, fixed_t y);
void P_Initsecnode(void);

void P_RadiusAttack(mobj_t *spot, mobj_t *source, fixed_t damagedist);

fixed_t P_FloorzAtPos(fixed_t x, fixed_t y, fixed_t z, fixed_t height);
boolean PIT_PushableMoved(mobj_t *thing);

void P_DoSpring(mobj_t *spring, mobj_t *object);

//
// P_SETUP
//
extern UINT8 *rejectmatrix; // for fast sight rejection
extern INT32 *blockmaplump; // offsets in blockmap are from here
extern INT32 *blockmap; // Big blockmap
extern INT32 bmapwidth;
extern INT32 bmapheight; // in mapblocks
extern fixed_t bmaporgx;
extern fixed_t bmaporgy; // origin of block map
extern mobj_t **blocklinks; // for thing chains

//
// P_INTER
//
typedef struct BasicFF_s
{
	INT32 ForceX; ///< The X of the Force's Vel
	INT32 ForceY; ///< The Y of the Force's Vel
	const player_t *player; ///< Player of Rumble
	//All
	UINT32 Duration; ///< The total duration of the effect, in microseconds
	INT32 Gain; ///< /The gain to be applied to the effect, in the range from 0 through 10,000.
	//All, CONSTANTFORCE �10,000 to 10,000
	INT32 Magnitude; ///< Magnitude of the effect, in the range from 0 through 10,000.
} BasicFF_t;

void P_ForceFeed(const player_t *player, INT32 attack, INT32 fade, tic_t duration, INT32 period);
void P_ForceConstant(const BasicFF_t *FFInfo);
void P_RampConstant(const BasicFF_t *FFInfo, INT32 Start, INT32 End);
void P_RemoveShield(player_t *player);
boolean P_DamageMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage);
void P_KillMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source);
void P_PlayerRingBurst(player_t *player, INT32 num_rings); /// \todo better fit in p_user.c
void P_PlayerWeaponPanelBurst(player_t *player);
void P_PlayerWeaponAmmoBurst(player_t *player);
void P_PlayerEmeraldBurst(player_t *player, boolean toss);

void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher, boolean heightcheck);
void P_PlayerFlagBurst(player_t *player, boolean toss);
void P_CheckPointLimit(void);
void P_CheckSurvivors(void);
boolean P_CheckRacers(void);

void P_ClearStarPost(INT32 postnum);
void P_ResetStarposts(void);

boolean P_CanPickupItem(player_t *player, boolean weapon);
void P_DoNightsScore(player_t *player);

//
// P_SPEC
//
#include "p_spec.h"

extern INT32 ceilmovesound;

// Factor to scale scrolling effect into mobj-carrying properties = 3/32.
// (This is so scrolling floors and objects on them can move at same speed.)
#define CARRYFACTOR ((3*FRACUNIT)/32)

void P_MixUp(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z, angle_t angle,
			INT16 starpostx, INT16 starposty, INT16 starpostz,
			INT32 starpostnum, tic_t starposttime, angle_t starpostangle,
			INT32 flags2);
boolean P_Teleport(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z, angle_t angle, boolean flash, boolean dontstopmove);
boolean P_SetMobjStateNF(mobj_t *mobj, statenum_t state);
boolean P_CheckMissileSpawn(mobj_t *th);
void P_Thrust(mobj_t *mo, angle_t angle, fixed_t move);
void P_DoSuperTransformation(player_t *player, boolean giverings);
void P_ExplodeMissile(mobj_t *mo);
void P_CheckGravity(mobj_t *mo, boolean affect);

#endif // __P_LOCAL__
