// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  info.h
/// \brief Thing frame/state LUT

#ifndef __INFO__
#define __INFO__

// Needed for action function pointer handling.
#include "d_think.h"
#include "sounds.h"
#include "m_fixed.h"
#include "dehacked.h" // MAX_ACTION_RECURSION

// deh_tables.c now has lists for the more named enums! PLEASE keep them up to date!
// For great modding!!

// IMPORTANT!
// DO NOT FORGET TO SYNC THIS LIST WITH THE ACTIONPOINTERS ARRAY IN DEH_TABLES.C
enum actionnum
{
	A_EXPLODE = 0,
	A_PAIN,
	A_FALL,
	A_MONITORPOP,
	A_GOLDMONITORPOP,
	A_GOLDMONITORRESTORE,
	A_GOLDMONITORSPARKLE,
	A_LOOK,
	A_CHASE,
	A_FACESTABCHASE,
	A_FACESTABREV,
	A_FACESTABHURL,
	A_FACESTABMISS,
	A_STATUEBURST,
	A_FACETARGET,
	A_FACETRACER,
	A_SCREAM,
	A_BOSSDEATH,
	A_SETSHADOWSCALE,
	A_SHADOWSCREAM,
	A_CUSTOMPOWER,
	A_GIVEWEAPON,
	A_RINGBOX,
	A_INVINCIBILITY,
	A_SUPERSNEAKERS,
	A_BUNNYHOP,
	A_BUBBLESPAWN,
	A_FANBUBBLESPAWN,
	A_BUBBLERISE,
	A_BUBBLECHECK,
	A_AWARDSCORE,
	A_EXTRALIFE,
	A_GIVESHIELD,
	A_GRAVITYBOX,
	A_SCORERISE,
	A_ATTRACTCHASE,
	A_DROPMINE,
	A_FISHJUMP,
	A_THROWNRING,
	A_SETSOLIDSTEAM,
	A_UNSETSOLIDSTEAM,
	A_SIGNSPIN,
	A_SIGNPLAYER,
	A_OVERLAYTHINK,
	A_JETCHASE,
	A_JETBTHINK,
	A_JETGTHINK,
	A_JETGSHOOT,
	A_SHOOTBULLET,
	A_MINUSDIGGING,
	A_MINUSPOPUP,
	A_MINUSCHECK,
	A_CHICKENCHECK,
	A_MOUSETHINK,
	A_DETONCHASE,
	A_CAPECHASE,
	A_ROTATESPIKEBALL,
	A_SLINGAPPEAR,
	A_UNIDUSBALL,
	A_ROCKSPAWN,
	A_SETFUSE,
	A_CRAWLACOMMANDERTHINK,
	A_SMOKETRAILER,
	A_RINGEXPLODE,
	A_OLDRINGEXPLODE,
	A_MIXUP,
	A_RECYCLEPOWERS,
	A_BOSS1CHASE,
	A_FOCUSTARGET,
	A_BOSS2CHASE,
	A_BOSS2POGO,
	A_BOSSZOOM,
	A_BOSSSCREAM,
	A_BOSS2TAKEDAMAGE,
	A_BOSS7CHASE,
	A_GOOPSPLAT,
	A_BOSS2POGOSFX,
	A_BOSS2POGOTARGET,
	A_BOSSJETFUME,
	A_EGGMANBOX,
	A_TURRETFIRE,
	A_SUPERTURRETFIRE,
	A_TURRETSTOP,
	A_JETJAWROAM,
	A_JETJAWCHOMP,
	A_POINTYTHINK,
	A_CHECKBUDDY,
	A_HOODFIRE,
	A_HOODTHINK,
	A_HOODFALL,
	A_ARROWBONKS,
	A_SNAILERTHINK,
	A_SHARPCHASE,
	A_SHARPSPIN,
	A_SHARPDECEL,
	A_CRUSHSTACEANWALK,
	A_CRUSHSTACEANPUNCH,
	A_CRUSHCLAWAIM,
	A_CRUSHCLAWLAUNCH,
	A_VULTUREVTOL,
	A_VULTURECHECK,
	A_VULTUREHOVER,
	A_VULTUREBLAST,
	A_VULTUREFLY,
	A_SKIMCHASE,
	A_1UPTHINKER,
	A_SKULLATTACK,
	A_LOBSHOT,
	A_FIRESHOT,
	A_SUPERFIRESHOT,
	A_BOSSFIRESHOT,
	A_BOSS7FIREMISSILES,
	A_BOSS1LASER,
	A_BOSS4REVERSE,
	A_BOSS4SPEEDUP,
	A_BOSS4RAISE,
	A_SPARKFOLLOW,
	A_BUZZFLY,
	A_GUARDCHASE,
	A_EGGSHIELD,
	A_SETREACTIONTIME,
	A_BOSS1SPIKEBALLS,
	A_BOSS3TAKEDAMAGE,
	A_BOSS3PATH,
	A_BOSS3SHOCKTHINK,
	A_SHOCKWAVE,
	A_LINEDEFEXECUTE,
	A_LINEDEFEXECUTEFROMARG,
	A_PLAYSEESOUND,
	A_PLAYATTACKSOUND,
	A_PLAYACTIVESOUND,
	A_SPAWNOBJECTABSOLUTE,
	A_SPAWNOBJECTRELATIVE,
	A_CHANGEANGLERELATIVE,
	A_CHANGEANGLEABSOLUTE,
	A_ROLLANGLE,
	A_CHANGEROLLANGLERELATIVE,
	A_CHANGEROLLANGLEABSOLUTE,
	A_PLAYSOUND,
	A_FINDTARGET,
	A_FINDTRACER,
	A_SETTICS,
	A_SETRANDOMTICS,
	A_CHANGECOLORRELATIVE,
	A_CHANGECOLORABSOLUTE,
	A_DYE,
	A_SETTRANSLATION,
	A_MOVERELATIVE,
	A_MOVEABSOLUTE,
	A_THRUST,
	A_ZTHRUST,
	A_SETTARGETSTARGET,
	A_SETOBJECTFLAGS,
	A_SETOBJECTFLAGS2,
	A_RANDOMSTATE,
	A_RANDOMSTATERANGE,
	A_STATERANGEBYANGLE,
	A_STATERANGEBYPARAMETER,
	A_DUALACTION,
	A_REMOTEACTION,
	A_TOGGLEFLAMEJET,
	A_ORBITNIGHTS,
	A_GHOSTME,
	A_SETOBJECTSTATE,
	A_SETOBJECTTYPESTATE,
	A_KNOCKBACK,
	A_PUSHAWAY,
	A_RINGDRAIN,
	A_SPLITSHOT,
	A_MISSILESPLIT,
	A_MULTISHOT,
	A_INSTALOOP,
	A_CUSTOM3DROTATE,
	A_SEARCHFORPLAYERS,
	A_CHECKRANDOM,
	A_CHECKTARGETRINGS,
	A_CHECKRINGS,
	A_CHECKTOTALRINGS,
	A_CHECKHEALTH,
	A_CHECKRANGE,
	A_CHECKHEIGHT,
	A_CHECKTRUERANGE,
	A_CHECKTHINGCOUNT,
	A_CHECKAMBUSH,
	A_CHECKCUSTOMVALUE,
	A_CHECKCUSVALMEMO,
	A_SETCUSTOMVALUE,
	A_USECUSVALMEMO,
	A_RELAYCUSTOMVALUE,
	A_CUSVALACTION,
	A_FORCESTOP,
	A_FORCEWIN,
	A_SPIKERETRACT,
	A_INFOSTATE,
	A_REPEAT,
	A_SETSCALE,
	A_REMOTEDAMAGE,
	A_HOMINGCHASE,
	A_TRAPSHOT,
	A_VILETARGET,
	A_VILEATTACK,
	A_VILEFIRE,
	A_BRAKCHASE,
	A_BRAKFIRESHOT,
	A_BRAKLOBSHOT,
	A_NAPALMSCATTER,
	A_SPAWNFRESHCOPY,
	A_FLICKYSPAWN,
	A_FLICKYCENTER,
	A_FLICKYAIM,
	A_FLICKYFLY,
	A_FLICKYSOAR,
	A_FLICKYCOAST,
	A_FLICKYHOP,
	A_FLICKYFLOUNDER,
	A_FLICKYCHECK,
	A_FLICKYHEIGHTCHECK,
	A_FLICKYFLUTTER,
	A_FLAMEPARTICLE,
	A_FADEOVERLAY,
	A_BOSS5JUMP,
	A_LIGHTBEAMRESET,
	A_MINEEXPLODE,
	A_MINERANGE,
	A_CONNECTTOGROUND,
	A_SPAWNPARTICLERELATIVE,
	A_MULTISHOTDIST,
	A_WHOCARESIFYOURSONISABEE,
	A_PARENTTRIESTOSLEEP,
	A_CRYINGTOMOMMA,
	A_CHECKFLAGS2,
	A_BOSS5FINDWAYPOINT,
	A_DONPCSKID,
	A_DONPCPAIN,
	A_PREPAREREPEAT,
	A_BOSS5EXTRAREPEAT,
	A_BOSS5CALM,
	A_BOSS5CHECKONGROUND,
	A_BOSS5CHECKFALLING,
	A_BOSS5PINCHSHOT,
	A_BOSS5MAKEITRAIN,
	A_BOSS5MAKEJUNK,
	A_LOOKFORBETTER,
	A_BOSS5BOMBEXPLODE,
	A_DUSTDEVILTHINK,
	A_TNTEXPLODE,
	A_DEBRISRANDOM,
	A_TRAINCAMEO,
	A_TRAINCAMEO2,
	A_CANARIVOREGAS,
	A_KILLSEGMENTS,
	A_SNAPPERSPAWN,
	A_SNAPPERTHINKER,
	A_SALOONDOORSPAWN,
	A_MINECARTSPARKTHINK,
	A_MODULOTOSTATE,
	A_LAVAFALLROCKS,
	A_LAVAFALLLAVA,
	A_FALLINGLAVACHECK,
	A_FIRESHRINK,
	A_SPAWNPTERABYTES,
	A_PTERABYTEHOVER,
	A_ROLLOUTSPAWN,
	A_ROLLOUTROCK,
	A_DRAGONBOMBERSPAWN,
	A_DRAGONWING,
	A_DRAGONSEGMENT,
	A_CHANGEHEIGHT,
	NUMACTIONS
};

struct mobj_s;

// IMPORTANT NOTE: If you add/remove from this list of action
// functions, don't forget to update them in deh_tables.c!
void A_Explode(struct mobj_s *actor);
void A_Pain(struct mobj_s *actor);
void A_Fall(struct mobj_s *actor);
void A_MonitorPop(struct mobj_s *actor);
void A_GoldMonitorPop(struct mobj_s *actor);
void A_GoldMonitorRestore(struct mobj_s *actor);
void A_GoldMonitorSparkle(struct mobj_s *actor);
void A_Look(struct mobj_s *actor);
void A_Chase(struct mobj_s *actor);
void A_FaceStabChase(struct mobj_s *actor);
void A_FaceStabRev(struct mobj_s *actor);
void A_FaceStabHurl(struct mobj_s *actor);
void A_FaceStabMiss(struct mobj_s *actor);
void A_StatueBurst(struct mobj_s *actor);
void A_FaceTarget(struct mobj_s *actor);
void A_FaceTracer(struct mobj_s *actor);
void A_Scream(struct mobj_s *actor);
void A_BossDeath(struct mobj_s *actor);
void A_SetShadowScale(struct mobj_s *actor);
void A_ShadowScream(struct mobj_s *actor); // MARIA!!!!!!
void A_CustomPower(struct mobj_s *actor); // Use this for a custom power
void A_GiveWeapon(struct mobj_s *actor); // Gives the player weapon(s)
void A_RingBox(struct mobj_s *actor); // Obtained Ring Box Tails
void A_Invincibility(struct mobj_s *actor); // Obtained Invincibility Box
void A_SuperSneakers(struct mobj_s *actor); // Obtained Super Sneakers Box
void A_BunnyHop(struct mobj_s *actor); // have bunny hop tails
void A_BubbleSpawn(struct mobj_s *actor); // Randomly spawn bubbles
void A_FanBubbleSpawn(struct mobj_s *actor);
void A_BubbleRise(struct mobj_s *actor); // Bubbles float to surface
void A_BubbleCheck(struct mobj_s *actor); // Don't draw if not underwater
void A_AwardScore(struct mobj_s *actor);
void A_ExtraLife(struct mobj_s *actor); // Extra Life
void A_GiveShield(struct mobj_s *actor); // Obtained Shield
void A_GravityBox(struct mobj_s *actor);
void A_ScoreRise(struct mobj_s *actor); // Rise the score logo
void A_AttractChase(struct mobj_s *actor); // Ring Chase
void A_DropMine(struct mobj_s *actor); // Drop Mine from Skim or Jetty-Syn Bomber
void A_FishJump(struct mobj_s *actor); // Fish Jump
void A_ThrownRing(struct mobj_s *actor); // Sparkle trail for red ring
void A_SetSolidSteam(struct mobj_s *actor);
void A_UnsetSolidSteam(struct mobj_s *actor);
void A_SignSpin(struct mobj_s *actor);
void A_SignPlayer(struct mobj_s *actor);
void A_OverlayThink(struct mobj_s *actor);
void A_JetChase(struct mobj_s *actor);
void A_JetbThink(struct mobj_s *actor); // Jetty-Syn Bomber Thinker
void A_JetgThink(struct mobj_s *actor); // Jetty-Syn Gunner Thinker
void A_JetgShoot(struct mobj_s *actor); // Jetty-Syn Shoot Function
void A_ShootBullet(struct mobj_s *actor); // JetgShoot without reactiontime setting
void A_MinusDigging(struct mobj_s *actor);
void A_MinusPopup(struct mobj_s *actor);
void A_MinusCheck(struct mobj_s *actor);
void A_ChickenCheck(struct mobj_s *actor);
void A_MouseThink(struct mobj_s *actor); // Mouse Thinker
void A_DetonChase(struct mobj_s *actor); // Deton Chaser
void A_CapeChase(struct mobj_s *actor); // Fake little Super Sonic cape
void A_RotateSpikeBall(struct mobj_s *actor); // Spike ball rotation
void A_SlingAppear(struct mobj_s *actor);
void A_UnidusBall(struct mobj_s *actor);
void A_RockSpawn(struct mobj_s *actor);
void A_SetFuse(struct mobj_s *actor);
void A_CrawlaCommanderThink(struct mobj_s *actor); // Crawla Commander
void A_SmokeTrailer(struct mobj_s *actor);
void A_RingExplode(struct mobj_s *actor);
void A_OldRingExplode(struct mobj_s *actor);
void A_MixUp(struct mobj_s *actor);
void A_RecyclePowers(struct mobj_s *actor);
void A_BossScream(struct mobj_s *actor);
void A_Boss2TakeDamage(struct mobj_s *actor);
void A_GoopSplat(struct mobj_s *actor);
void A_Boss2PogoSFX(struct mobj_s *actor);
void A_Boss2PogoTarget(struct mobj_s *actor);
void A_EggmanBox(struct mobj_s *actor);
void A_TurretFire(struct mobj_s *actor);
void A_SuperTurretFire(struct mobj_s *actor);
void A_TurretStop(struct mobj_s *actor);
void A_JetJawRoam(struct mobj_s *actor);
void A_JetJawChomp(struct mobj_s *actor);
void A_PointyThink(struct mobj_s *actor);
void A_CheckBuddy(struct mobj_s *actor);
void A_HoodFire(struct mobj_s *actor);
void A_HoodThink(struct mobj_s *actor);
void A_HoodFall(struct mobj_s *actor);
void A_ArrowBonks(struct mobj_s *actor);
void A_SnailerThink(struct mobj_s *actor);
void A_SharpChase(struct mobj_s *actor);
void A_SharpSpin(struct mobj_s *actor);
void A_SharpDecel(struct mobj_s *actor);
void A_CrushstaceanWalk(struct mobj_s *actor);
void A_CrushstaceanPunch(struct mobj_s *actor);
void A_CrushclawAim(struct mobj_s *actor);
void A_CrushclawLaunch(struct mobj_s *actor);
void A_VultureVtol(struct mobj_s *actor);
void A_VultureCheck(struct mobj_s *actor);
void A_VultureHover(struct mobj_s *actor);
void A_VultureBlast(struct mobj_s *actor);
void A_VultureFly(struct mobj_s *actor);
void A_SkimChase(struct mobj_s *actor);
void A_SkullAttack(struct mobj_s *actor);
void A_LobShot(struct mobj_s *actor);
void A_FireShot(struct mobj_s *actor);
void A_SuperFireShot(struct mobj_s *actor);
void A_BossFireShot(struct mobj_s *actor);
void A_Boss7FireMissiles(struct mobj_s *actor);
void A_Boss1Laser(struct mobj_s *actor);
void A_FocusTarget(struct mobj_s *actor);
void A_Boss4Reverse(struct mobj_s *actor);
void A_Boss4SpeedUp(struct mobj_s *actor);
void A_Boss4Raise(struct mobj_s *actor);
void A_SparkFollow(struct mobj_s *actor);
void A_BuzzFly(struct mobj_s *actor);
void A_GuardChase(struct mobj_s *actor);
void A_EggShield(struct mobj_s *actor);
void A_SetReactionTime(struct mobj_s *actor);
void A_Boss1Spikeballs(struct mobj_s *actor);
void A_Boss3TakeDamage(struct mobj_s *actor);
void A_Boss3Path(struct mobj_s *actor);
void A_Boss3ShockThink(struct mobj_s *actor);
void A_Shockwave(struct mobj_s *actor);
void A_LinedefExecute(struct mobj_s *actor);
void A_LinedefExecuteFromArg(struct mobj_s *actor);
void A_PlaySeeSound(struct mobj_s *actor);
void A_PlayAttackSound(struct mobj_s *actor);
void A_PlayActiveSound(struct mobj_s *actor);
void A_1upThinker(struct mobj_s *actor);
void A_BossZoom(struct mobj_s *actor); //Unused
void A_Boss1Chase(struct mobj_s *actor);
void A_Boss2Chase(struct mobj_s *actor);
void A_Boss2Pogo(struct mobj_s *actor);
void A_Boss7Chase(struct mobj_s *actor);
void A_BossJetFume(struct mobj_s *actor);
void A_SpawnObjectAbsolute(struct mobj_s *actor);
void A_SpawnObjectRelative(struct mobj_s *actor);
void A_ChangeAngleRelative(struct mobj_s *actor);
void A_ChangeAngleAbsolute(struct mobj_s *actor);
void A_RollAngle(struct mobj_s *actor);
void A_ChangeRollAngleRelative(struct mobj_s *actor);
void A_ChangeRollAngleAbsolute(struct mobj_s *actor);
void A_PlaySound(struct mobj_s *actor);
void A_FindTarget(struct mobj_s *actor);
void A_FindTracer(struct mobj_s *actor);
void A_SetTics(struct mobj_s *actor);
void A_SetRandomTics(struct mobj_s *actor);
void A_ChangeColorRelative(struct mobj_s *actor);
void A_ChangeColorAbsolute(struct mobj_s *actor);
void A_Dye(struct mobj_s *actor);
void A_SetTranslation(struct mobj_s *actor);
void A_MoveRelative(struct mobj_s *actor);
void A_MoveAbsolute(struct mobj_s *actor);
void A_Thrust(struct mobj_s *actor);
void A_ZThrust(struct mobj_s *actor);
void A_SetTargetsTarget(struct mobj_s *actor);
void A_SetObjectFlags(struct mobj_s *actor);
void A_SetObjectFlags2(struct mobj_s *actor);
void A_RandomState(struct mobj_s *actor);
void A_RandomStateRange(struct mobj_s *actor);
void A_StateRangeByAngle(struct mobj_s *actor);
void A_StateRangeByParameter(struct mobj_s *actor);
void A_DualAction(struct mobj_s *actor);
void A_RemoteAction(struct mobj_s *actor);
void A_ToggleFlameJet(struct mobj_s *actor);
void A_OrbitNights(struct mobj_s *actor);
void A_GhostMe(struct mobj_s *actor);
void A_SetObjectState(struct mobj_s *actor);
void A_SetObjectTypeState(struct mobj_s *actor);
void A_KnockBack(struct mobj_s *actor);
void A_PushAway(struct mobj_s *actor);
void A_RingDrain(struct mobj_s *actor);
void A_SplitShot(struct mobj_s *actor);
void A_MissileSplit(struct mobj_s *actor);
void A_MultiShot(struct mobj_s *actor);
void A_InstaLoop(struct mobj_s *actor);
void A_Custom3DRotate(struct mobj_s *actor);
void A_SearchForPlayers(struct mobj_s *actor);
void A_CheckRandom(struct mobj_s *actor);
void A_CheckTargetRings(struct mobj_s *actor);
void A_CheckRings(struct mobj_s *actor);
void A_CheckTotalRings(struct mobj_s *actor);
void A_CheckHealth(struct mobj_s *actor);
void A_CheckRange(struct mobj_s *actor);
void A_CheckHeight(struct mobj_s *actor);
void A_CheckTrueRange(struct mobj_s *actor);
void A_CheckThingCount(struct mobj_s *actor);
void A_CheckAmbush(struct mobj_s *actor);
void A_CheckCustomValue(struct mobj_s *actor);
void A_CheckCusValMemo(struct mobj_s *actor);
void A_SetCustomValue(struct mobj_s *actor);
void A_UseCusValMemo(struct mobj_s *actor);
void A_RelayCustomValue(struct mobj_s *actor);
void A_CusValAction(struct mobj_s *actor);
void A_ForceStop(struct mobj_s *actor);
void A_ForceWin(struct mobj_s *actor);
void A_SpikeRetract(struct mobj_s *actor);
void A_InfoState(struct mobj_s *actor);
void A_Repeat(struct mobj_s *actor);
void A_SetScale(struct mobj_s *actor);
void A_RemoteDamage(struct mobj_s *actor);
void A_HomingChase(struct mobj_s *actor);
void A_TrapShot(struct mobj_s *actor);
void A_VileTarget(struct mobj_s *actor);
void A_VileAttack(struct mobj_s *actor);
void A_VileFire(struct mobj_s *actor);
void A_BrakChase(struct mobj_s *actor);
void A_BrakFireShot(struct mobj_s *actor);
void A_BrakLobShot(struct mobj_s *actor);
void A_NapalmScatter(struct mobj_s *actor);
void A_SpawnFreshCopy(struct mobj_s *actor);
void A_FlickySpawn(struct mobj_s *actor);
void A_FlickyCenter(struct mobj_s *actor);
void A_FlickyAim(struct mobj_s *actor);
void A_FlickyFly(struct mobj_s *actor);
void A_FlickySoar(struct mobj_s *actor);
void A_FlickyCoast(struct mobj_s *actor);
void A_FlickyHop(struct mobj_s *actor);
void A_FlickyFlounder(struct mobj_s *actor);
void A_FlickyCheck(struct mobj_s *actor);
void A_FlickyHeightCheck(struct mobj_s *actor);
void A_FlickyFlutter(struct mobj_s *actor);
void A_FlameParticle(struct mobj_s *actor);
void A_FadeOverlay(struct mobj_s *actor);
void A_Boss5Jump(struct mobj_s *actor);
void A_LightBeamReset(struct mobj_s *actor);
void A_MineExplode(struct mobj_s *actor);
void A_MineRange(struct mobj_s *actor);
void A_ConnectToGround(struct mobj_s *actor);
void A_SpawnParticleRelative(struct mobj_s *actor);
void A_MultiShotDist(struct mobj_s *actor);
void A_WhoCaresIfYourSonIsABee(struct mobj_s *actor);
void A_ParentTriesToSleep(struct mobj_s *actor);
void A_CryingToMomma(struct mobj_s *actor);
void A_CheckFlags2(struct mobj_s *actor);
void A_Boss5FindWaypoint(struct mobj_s *actor);
void A_DoNPCSkid(struct mobj_s *actor);
void A_DoNPCPain(struct mobj_s *actor);
void A_PrepareRepeat(struct mobj_s *actor);
void A_Boss5ExtraRepeat(struct mobj_s *actor);
void A_Boss5Calm(struct mobj_s *actor);
void A_Boss5CheckOnGround(struct mobj_s *actor);
void A_Boss5CheckFalling(struct mobj_s *actor);
void A_Boss5PinchShot(struct mobj_s *actor);
void A_Boss5MakeItRain(struct mobj_s *actor);
void A_Boss5MakeJunk(struct mobj_s *actor);
void A_LookForBetter(struct mobj_s *actor);
void A_Boss5BombExplode(struct mobj_s *actor);
void A_DustDevilThink(struct mobj_s *actor);
void A_TNTExplode(struct mobj_s *actor);
void A_DebrisRandom(struct mobj_s *actor);
void A_TrainCameo(struct mobj_s *actor);
void A_TrainCameo2(struct mobj_s *actor);
void A_CanarivoreGas(struct mobj_s *actor);
void A_KillSegments(struct mobj_s *actor);
void A_SnapperSpawn(struct mobj_s *actor);
void A_SnapperThinker(struct mobj_s *actor);
void A_SaloonDoorSpawn(struct mobj_s *actor);
void A_MinecartSparkThink(struct mobj_s *actor);
void A_ModuloToState(struct mobj_s *actor);
void A_LavafallRocks(struct mobj_s *actor);
void A_LavafallLava(struct mobj_s *actor);
void A_FallingLavaCheck(struct mobj_s *actor);
void A_FireShrink(struct mobj_s *actor);
void A_SpawnPterabytes(struct mobj_s *actor);
void A_PterabyteHover(struct mobj_s *actor);
void A_RolloutSpawn(struct mobj_s *actor);
void A_RolloutRock(struct mobj_s *actor);
void A_DragonbomberSpawn(struct mobj_s *actor);
void A_DragonWing(struct mobj_s *actor);
void A_DragonSegment(struct mobj_s *actor);
void A_ChangeHeight(struct mobj_s *actor);

extern int actionsoverridden[NUMACTIONS][MAX_ACTION_RECURSION];

// ratio of states to sprites to mobj types is roughly 6 : 1 : 1
#define NUMMOBJTYPES 1024
#define NUMSPRITEFREESLOTS NUMMOBJTYPES
#define NUMSTATES (NUMMOBJTYPES*8)
#define MAXSPRITENAME 64

// Hey, moron! If you change this table, don't forget about sprnames in info.c and the sprite lights in hw_light.c!
typedef enum sprite
{
	SPR_NULL, // invisible object
	SPR_UNKN,

	SPR_THOK, // Thok! mobj
	SPR_PLAY,

	// Enemies
	SPR_POSS, // Crawla (Blue)
	SPR_SPOS, // Crawla (Red)
	SPR_FISH, // SDURF
	SPR_BUZZ, // Buzz (Gold)
	SPR_RBUZ, // Buzz (Red)
	SPR_JETB, // Jetty-Syn Bomber
	SPR_JETG, // Jetty-Syn Gunner
	SPR_CCOM, // Crawla Commander
	SPR_DETN, // Deton
	SPR_SKIM, // Skim mine dropper
	SPR_TRET, // Industrial Turret
	SPR_TURR, // Pop-Up Turret
	SPR_SHRP, // Sharp
	SPR_CRAB, // Crushstacean
	SPR_CR2B, // Banpyura
	SPR_CSPR, // Banpyura spring
	SPR_JJAW, // Jet Jaw
	SPR_SNLR, // Snailer
	SPR_VLTR, // BASH
	SPR_PNTY, // Pointy
	SPR_ARCH, // Robo-Hood
	SPR_CBFS, // Castlebot Facestabber
	SPR_STAB, // Castlebot Facestabber spear aura
	SPR_SPSH, // Egg Guard
	SPR_ESHI, // Egg Guard's shield
	SPR_GSNP, // Green Snapper
	SPR_GSNL, // Green Snapper leg
	SPR_GSNH, // Green Snapper head
	SPR_MNUS, // Minus
	SPR_MNUD, // Minus dirt
	SPR_SSHL, // Spring Shell
	SPR_UNID, // Unidus
	SPR_CANA, // Canarivore
	SPR_CANG, // Canarivore gas
	SPR_PYRE, // Pyre Fly
	SPR_PTER, // Pterabyte
	SPR_DRAB, // Dragonbomber

	// Generic Boss Items
	SPR_JETF, // Boss jet fumes

	// Boss 1 (Greenflower)
	SPR_EGGM, // Boss 1
	SPR_EGLZ, // Boss 1 Junk

	// Boss 2 (Techno Hill)
	SPR_EGGN, // Boss 2
	SPR_TANK, // Boss 2 Junk
	SPR_GOOP, // Boss 2 Goop

	// Boss 3 (Deep Sea)
	SPR_EGGO, // Boss 3
	SPR_SEBH, // Boss 3 Junk
	SPR_FAKE, // Boss 3 Fakemobile
	SPR_SHCK, // Boss 3 Shockwave

	// Boss 4 (Castle Eggman)
	SPR_EGGP,
	SPR_EFIR, // Boss 4 jet flame
	SPR_EGR1, // Boss 4 Spectator Eggrobo

	// Boss 5 (Arid Canyon)
	SPR_FANG, // replaces EGGQ
	SPR_BRKN,
	SPR_WHAT,
	SPR_VWRE,
	SPR_PROJ, // projector light
	SPR_FBOM,
	SPR_FSGN,
	SPR_BARX, // bomb explosion (also used by barrel)
	SPR_BARD, // bomb dust (also used by barrel)

	// Boss 6 (Red Volcano)
	SPR_EGGR,

	// Boss 7 (Dark City)
	SPR_BRAK,
	SPR_BGOO, // Goop
	SPR_BMSL,

	// Boss 8 (Egg Rock)
	SPR_EGGT,

	// Cy-Brak-Demon; uses SPR_BRAK as well, but has some extras
	SPR_RCKT, // Rockets!
	SPR_ELEC, // Electricity!
	SPR_TARG, // Targeting reticules!
	SPR_NPLM, // Big napalm bombs!
	SPR_MNPL, // Mini napalm bombs!

	// Metal Sonic
	SPR_METL,
	SPR_MSCF,
	SPR_MSCB,

	// Collectible Items
	SPR_RING,
	SPR_TRNG, // Team Rings
	SPR_TOKE, // Special Stage Token
	SPR_RFLG, // Red CTF Flag
	SPR_BFLG, // Blue CTF Flag
	SPR_SPHR, // Sphere
	SPR_NCHP, // NiGHTS chip
	SPR_NSTR, // NiGHTS star
	SPR_EMBM, // Emblem
	SPR_CEMG, // Chaos Emeralds
	SPR_SHRD, // Emerald Hunt

	// Interactive Objects
	SPR_BBLS, // water bubble source
	SPR_SIGN, // Level end sign
	SPR_SPIK, // Spike Ball
	SPR_SFLM, // Spin fire
	SPR_TFLM, // Spin fire (team)
	SPR_USPK, // Floor spike
	SPR_WSPK, // Wall spike
	SPR_WSPB, // Wall spike base
	SPR_STPT, // Starpost
	SPR_BMNE, // Big floating mine
	SPR_PUMI, // Rollout Rock

	// Monitor Boxes
	SPR_MSTV, // MiSc TV sprites
	SPR_XLTV, // eXtra Large TV sprites

	SPR_TRRI, // Red team:  10 RIngs
	SPR_TBRI, // Blue team: 10 RIngs

	SPR_TVRI, // 10 RIng
	SPR_TVPI, // PIty shield
	SPR_TVAT, // ATtraction shield
	SPR_TVFO, // FOrce shield
	SPR_TVAR, // ARmageddon shield
	SPR_TVWW, // WhirlWind shield
	SPR_TVEL, // ELemental shield
	SPR_TVSS, // Super Sneakers
	SPR_TVIV, // InVincibility
	SPR_TV1U, // 1Up
	SPR_TV1P, // 1uP (textless)
	SPR_TVEG, // EGgman
	SPR_TVMX, // MiXup
	SPR_TVMY, // MYstery
	SPR_TVGV, // GraVity boots
	SPR_TVRC, // ReCycler
	SPR_TV1K, // 1,000 points  (1 K)
	SPR_TVTK, // 10,000 points (Ten K)
	SPR_TVFL, // FLame shield
	SPR_TVBB, // BuBble shield
	SPR_TVZP, // Thunder shield (ZaP)

	// Projectiles
	SPR_MISL,
	SPR_LASR, // GFZ3 laser
	SPR_LASF, // GFZ3 laser flames
	SPR_TORP, // Torpedo
	SPR_ENRG, // Energy ball
	SPR_MINE, // Skim mine
	SPR_JBUL, // Jetty-Syn Bullet
	SPR_TRLS,
	SPR_CBLL, // Cannonball
	SPR_AROW, // Arrow
	SPR_CFIR, // Colored fire of various sorts

	// The letter
	SPR_LETR,

	// Tutorial scenery
	SPR_TUPL,
	SPR_TUPF,

	// Greenflower Scenery
	SPR_FWR1,
	SPR_FWR2, // GFZ Sunflower
	SPR_FWR3, // GFZ budding flower
	SPR_FWR4,
	SPR_BUS1, // GFZ Bush w/ berries
	SPR_BUS2, // GFZ Bush w/o berries
	SPR_BUS3, // GFZ Bush w/ BLUE berries
	// Trees (both GFZ and misc)
	SPR_TRE1, // GFZ
	SPR_TRE2, // Checker
	SPR_TRE3, // Frozen Hillside
	SPR_TRE4, // Polygon
	SPR_TRE5, // Bush tree
	SPR_TRE6, // Spring tree

	// Techno Hill Scenery
	SPR_THZP, // THZ1 Steam Flower
	SPR_FWR5, // THZ1 Spin flower (red)
	SPR_FWR6, // THZ1 Spin flower (yellow)
	SPR_THZT, // Steam Whistle tree/bush
	SPR_ALRM, // THZ2 Alarm

	// Deep Sea Scenery
	SPR_GARG, // Deep Sea Gargoyle
	SPR_SEWE, // Deep Sea Seaweed
	SPR_DRIP, // Dripping water
	SPR_CORL, // Coral
	SPR_BCRY, // Blue Crystal
	SPR_KELP, // Kelp
	SPR_ALGA, // Animated algae top
	SPR_ALGB, // Animated algae segment
	SPR_DSTG, // DSZ Stalagmites
	SPR_LIBE, // DSZ Light beam

	// Castle Eggman Scenery
	SPR_CHAN, // CEZ Chain
	SPR_FLAM, // Flame
	SPR_ESTA, // Eggman esta una estatua!
	SPR_SMCH, // Small Mace Chain
	SPR_BMCH, // Big Mace Chain
	SPR_SMCE, // Small Mace
	SPR_BMCE, // Big Mace
	SPR_BSPB, // Blue spring on a ball
	SPR_YSPB, // Yellow spring on a ball
	SPR_RSPB, // Red spring on a ball
	SPR_SFBR, // Small Firebar
	SPR_BFBR, // Big Firebar
	SPR_BANR, // Banner/pole
	SPR_PINE, // Pine Tree
	SPR_CEZB, // Bush
	SPR_CNDL, // Candle/pricket
	SPR_FLMH, // Flame holder
	SPR_CTRC, // Fire torch
	SPR_CFLG, // Waving flag/segment
	SPR_CSTA, // Crawla statue
	SPR_CBBS, // Facestabber statue
	SPR_CABR, // Brambles

	// Arid Canyon Scenery
	SPR_BTBL, // Big tumbleweed
	SPR_STBL, // Small tumbleweed
	SPR_CACT, // Cacti
	SPR_WWSG, // Caution Sign
	SPR_WWS2, // Cacti Sign
	SPR_WWS3, // Sharp Turn Sign
	SPR_OILL, // Oil lamp
	SPR_OILF, // Oil lamp flare
	SPR_BARR, // TNT barrel
	SPR_REMT, // TNT proximity shell
	SPR_TAZD, // Dust devil
	SPR_ADST, // Arid dust
	SPR_MCRT, // Minecart
	SPR_MCSP, // Minecart spark
	SPR_SALD, // Saloon door
	SPR_TRAE, // Train cameo locomotive
	SPR_TRAI, // Train cameo wagon
	SPR_STEA, // Train steam

	// Red Volcano Scenery
	SPR_FLME, // Flame jet
	SPR_DFLM, // Blade's flame
	SPR_LFAL, // Lavafall
	SPR_JPLA, // Jungle palm
	SPR_TFLO, // Torch flower
	SPR_WVIN, // Wall vines

	// Dark City Scenery

	// Egg Rock Scenery

	// Christmas Scenery
	SPR_XMS1, // Christmas Pole
	SPR_XMS2, // Candy Cane
	SPR_XMS3, // Snowman
	SPR_XMS4, // Lamppost
	SPR_XMS5, // Hanging Star
	SPR_XMS6, // Mistletoe
	SPR_SNTT, // Silver Shiver tree
	SPR_SSTT, // Silver Shiver tree with snow
	SPR_FHZI, // FHZ Ice
	SPR_ROSY,

	// Halloween Scenery
	SPR_PUMK, // Pumpkins
	SPR_HHPL, // Dr Seuss Trees
	SPR_SHRM, // Mushroom
	SPR_HHZM, // Misc

	// Azure Temple Scenery
	SPR_BGAR, // ATZ Gargoyles
	SPR_RCRY, // ATZ Red Crystal (Target)
	SPR_CFLM, // Green torch flame

	// Botanic Serenity Scenery
	SPR_BSZ1, // Tall flowers
	SPR_BSZ2, // Medium flowers
	SPR_BSZ3, // Small flowers
	//SPR_BSZ4, -- Tulips
	SPR_BST1, // Red tulip
	SPR_BST2, // Purple tulip
	SPR_BST3, // Blue tulip
	SPR_BST4, // Cyan tulip
	SPR_BST5, // Yellow tulip
	SPR_BST6, // Orange tulip
	SPR_BSZ5, // Cluster of Tulips
	SPR_BSZ6, // Bush
	SPR_BSZ7, // Vine
	SPR_BSZ8, // Misc things

	// Misc Scenery
	SPR_STLG, // Stalagmites
	SPR_DBAL, // Disco
	SPR_GINE, // Crystalline Heights tree
	SPR_PPAL, // Pristine Shores palm trees

	// Powerup Indicators
	SPR_ARMA, // Armageddon Shield Orb
	SPR_ARMF, // Armageddon Shield Ring, Front
	SPR_ARMB, // Armageddon Shield Ring, Back
	SPR_WIND, // Whirlwind Shield Orb
	SPR_MAGN, // Attract Shield Orb
	SPR_ELEM, // Elemental Shield Orb
	SPR_FORC, // Force Shield Orb
	SPR_PITY, // Pity Shield Orb
	SPR_FIRS, // Flame Shield Orb
	SPR_BUBS, // Bubble Shield Orb
	SPR_ZAPS, // Thunder Shield Orb
	SPR_IVSP, // invincibility sparkles
	SPR_SSPK, // Super Sonic Spark

	SPR_GOAL, // Special Stage goal (here because lol NiGHTS)

	// Flickies
	SPR_FBUB, // Flicky-sized bubble
	SPR_FL01, // Bluebird
	SPR_FL02, // Rabbit
	SPR_FL03, // Chicken
	SPR_FL04, // Seal
	SPR_FL05, // Pig
	SPR_FL06, // Chipmunk
	SPR_FL07, // Penguin
	SPR_FL08, // Fish
	SPR_FL09, // Ram
	SPR_FL10, // Puffin
	SPR_FL11, // Cow
	SPR_FL12, // Rat
	SPR_FL13, // Bear
	SPR_FL14, // Dove
	SPR_FL15, // Cat
	SPR_FL16, // Canary
	SPR_FS01, // Spider
	SPR_FS02, // Bat

	// Springs
	SPR_FANS, // Fan
	SPR_STEM, // Steam riser
	SPR_BUMP, // Bumpers
	SPR_BLON, // Balloons
	SPR_SPRY, // Yellow spring
	SPR_SPRR, // Red spring
	SPR_SPRB, // Blue spring
	SPR_YSPR, // Yellow Diagonal Spring
	SPR_RSPR, // Red Diagonal Spring
	SPR_BSPR, // Blue Diagonal Spring
	SPR_SSWY, // Yellow Side Spring
	SPR_SSWR, // Red Side Spring
	SPR_SSWB, // Blue Side Spring
	SPR_BSTY, // Yellow Booster
	SPR_BSTR, // Red Booster

	// Environmental Effects
	SPR_RAIN, // Rain
	SPR_SNO1, // Snowflake
	SPR_SPLH, // Water Splish
	SPR_LSPL, // Lava Splish
	SPR_SPLA, // Water Splash
	SPR_SMOK,
	SPR_BUBL, // Bubble
	SPR_WZAP,
	SPR_DUST, // Spindash dust
	SPR_FPRT, // Spindash dust (flame)
	SPR_TFOG, // Teleport Fog
	SPR_SEED, // Sonic CD flower seed
	SPR_PRTL, // Particle (for fans, etc.)

	// Game Indicators
	SPR_SCOR, // Score logo
	SPR_DRWN, // Drowning Timer
	SPR_FLII, // AI flight indicator
	SPR_LCKN, // Target
	SPR_TTAG, // Tag Sign
	SPR_GFLG, // Got Flag sign
	SPR_FNSF, // Finish flag

	SPR_CORK,
	SPR_LHRT,

	// Ring Weapons
	SPR_RRNG, // Red Ring
	SPR_RNGB, // Bounce Ring
	SPR_RNGR, // Rail Ring
	SPR_RNGI, // Infinity Ring
	SPR_RNGA, // Automatic Ring
	SPR_RNGE, // Explosion Ring
	SPR_RNGS, // Scatter Ring
	SPR_RNGG, // Grenade Ring

	SPR_PIKB, // Bounce Ring Pickup
	SPR_PIKR, // Rail Ring Pickup
	SPR_PIKA, // Automatic Ring Pickup
	SPR_PIKE, // Explosion Ring Pickup
	SPR_PIKS, // Scatter Ring Pickup
	SPR_PIKG, // Grenade Ring Pickup

	SPR_TAUT, // Thrown Automatic Ring
	SPR_TGRE, // Thrown Grenade Ring
	SPR_TSCR, // Thrown Scatter Ring

	// Mario-specific stuff
	SPR_COIN,
	SPR_CPRK,
	SPR_GOOM,
	SPR_BGOM,
	SPR_FFWR,
	SPR_FBLL,
	SPR_SHLL,
	SPR_PUMA,
	SPR_HAMM,
	SPR_KOOP,
	SPR_BFLM,
	SPR_MAXE,
	SPR_MUS1,
	SPR_MUS2,
	SPR_TOAD,

	// NiGHTS Stuff
	SPR_NDRN, // NiGHTS drone
	SPR_NSPK, // NiGHTS sparkle
	SPR_NBMP, // NiGHTS Bumper
	SPR_HOOP, // NiGHTS hoop sprite
	SPR_NSCR, // NiGHTS score sprite
	SPR_NPRU, // Nights Powerups
	SPR_CAPS, // Capsule thingy for NiGHTS
	SPR_IDYA, // Ideya
	SPR_NTPN, // Nightopian
	SPR_SHLP, // Shleep

	// Secret badniks and hazards, shhhh
	SPR_PENG,
	SPR_POPH,
	SPR_HIVE,
	SPR_BUMB,
	SPR_BBUZ,
	SPR_FMCE,
	SPR_HMCE,
	SPR_CACO,
	SPR_BAL2,
	SPR_SBOB,
	SPR_SBFL,
	SPR_SBSK,
	SPR_HBAT,

	// Debris
	SPR_SPRK, // Sparkle
	SPR_BOM1, // Robot Explosion
	SPR_BOM2, // Boss Explosion 1
	SPR_BOM3, // Boss Explosion 2
	SPR_BOM4, // Underwater Explosion
	SPR_BMNB, // Mine Explosion

	// Crumbly rocks
	SPR_ROIA,
	SPR_ROIB,
	SPR_ROIC,
	SPR_ROID,
	SPR_ROIE,
	SPR_ROIF,
	SPR_ROIG,
	SPR_ROIH,
	SPR_ROII,
	SPR_ROIJ,
	SPR_ROIK,
	SPR_ROIL,
	SPR_ROIM,
	SPR_ROIN,
	SPR_ROIO,
	SPR_ROIP,

	// Level debris
	SPR_GFZD, // GFZ debris
	SPR_BRIC, // Bricks
	SPR_WDDB, // Wood Debris
	SPR_BRIR, // CEZ3 colored bricks
	SPR_BRIB,
	SPR_BRIY,

	// Gravity Well Objects
	SPR_GWLG,
	SPR_GWLR,

	// LJ Knuckles
	SPR_OLDK,

	SPR_FIRSTFREESLOT,
	SPR_LASTFREESLOT = SPR_FIRSTFREESLOT + NUMSPRITEFREESLOTS - 1,
	NUMSPRITES
} spritenum_t;

typedef enum playersprite
{
	SPR2_STND = 0,
	SPR2_WAIT,
	SPR2_WALK,
	SPR2_SKID,
	SPR2_RUN ,
	SPR2_DASH,
	SPR2_PAIN,
	SPR2_STUN,
	SPR2_DEAD,
	SPR2_DRWN, // drown
	SPR2_ROLL,
	SPR2_GASP,
	SPR2_JUMP,
	SPR2_SPNG, // spring
	SPR2_FALL,
	SPR2_EDGE,
	SPR2_RIDE,

	SPR2_SPIN, // spindash

	SPR2_FLY ,
	SPR2_SWIM,
	SPR2_TIRE, // tired

	SPR2_GLID, // glide
	SPR2_LAND, // landing after glide/bounce
	SPR2_CLNG, // cling
	SPR2_CLMB, // climb

	SPR2_FLT , // float
	SPR2_FRUN, // float run

	SPR2_BNCE, // bounce

	SPR2_FIRE, // fire

	SPR2_TWIN, // twinspin

	SPR2_MLEE, // melee
	SPR2_MLEL, // melee land

	SPR2_TRNS, // transformation

	SPR2_NSTD, // NiGHTS stand
	SPR2_NFLT, // NiGHTS float
	SPR2_NFLY, // NiGHTS fly
	SPR2_NDRL, // NiGHTS drill
	SPR2_NSTN, // NiGHTS stun
	SPR2_NPUL, // NiGHTS pull
	SPR2_NATK, // NiGHTS attack

	// c:
	SPR2_TAL0,
	SPR2_TAL1,
	SPR2_TAL2,
	SPR2_TAL3,
	SPR2_TAL4,
	SPR2_TAL5,
	SPR2_TAL6,
	SPR2_TAL7,
	SPR2_TAL8,
	SPR2_TAL9,
	SPR2_TALA,
	SPR2_TALB,
	SPR2_TALC,

	// Misc slots
	SPR2_MSC0,
	SPR2_MSC1,
	SPR2_MSC2,
	SPR2_MSC3,
	SPR2_MSC4,
	SPR2_MSC5,
	SPR2_MSC6,
	SPR2_MSC7,
	SPR2_MSC8,
	SPR2_MSC9,

	SPR2_CNT1, // continue disappointment
	SPR2_CNT2, // continue lift
	SPR2_CNT3, // continue spin
	SPR2_CNT4, // continue "soooooooniiic!" tugging

	SPR2_SIGN, // end sign head
	SPR2_LIFE, // life monitor icon

	SPR2_XTRA, // stuff that isn't in-map - "would this ever need an md2 or variable length animation?"

	SPR2_FIRSTFREESLOT,
	SPR2_LASTFREESLOT = 1024, // Do not make higher than SPR2F_MASK (currently 0x3FF) plus one
	NUMPLAYERSPRITES
} playersprite_t;

enum
{
	XTRA_LIFEPIC,
	XTRA_CHARSEL,
	XTRA_CONTINUE,
	XTRA_ENDING
};

typedef INT32 statenum_t;

typedef struct
{
	spritenum_t sprite;
	UINT32 frame; // we use the upper 16 bits for translucency and other shade effects
	INT32 tics;
	actionf_t action;
	INT32 var1;
	INT32 var2;
	statenum_t nextstate;
	UINT16 sprite2;
} state_t;

extern state_t states[NUMSTATES];
extern char sprnames[NUMSPRITES + 1][MAXSPRITENAME + 1];
extern char spr2names[NUMPLAYERSPRITES][MAXSPRITENAME + 1];
extern playersprite_t spr2defaults[NUMPLAYERSPRITES];
extern state_t *astate;
extern playersprite_t free_spr2;

enum
{
	MT_NULL,
	MT_UNKNOWN,

	MT_THOK, // Thok! mobj
	MT_PLAYER,
	MT_TAILSOVERLAY, // c:
	MT_METALJETFUME,

	// Enemies
	MT_BLUECRAWLA, // Crawla (Blue)
	MT_REDCRAWLA, // Crawla (Red)
	MT_GFZFISH, // SDURF
	MT_GOLDBUZZ, // Buzz (Gold)
	MT_REDBUZZ, // Buzz (Red)
	MT_JETTBOMBER, // Jetty-Syn Bomber
	MT_JETTGUNNER, // Jetty-Syn Gunner
	MT_CRAWLACOMMANDER, // Crawla Commander
	MT_DETON, // Deton
	MT_SKIM, // Skim mine dropper
	MT_TURRET, // Industrial Turret
	MT_POPUPTURRET, // Pop-Up Turret
	MT_SPINCUSHION, // Spincushion
	MT_CRUSHSTACEAN, // Crushstacean
	MT_CRUSHCLAW, // Big meaty claw
	MT_CRUSHCHAIN, // Chain
	MT_BANPYURA, // Banpyura
	MT_BANPSPRING, // Banpyura spring
	MT_JETJAW, // Jet Jaw
	MT_SNAILER, // Snailer
	MT_VULTURE, // BASH
	MT_POINTY, // Pointy
	MT_POINTYBALL, // Pointy Ball
	MT_ROBOHOOD, // Robo-Hood
	MT_FACESTABBER, // Castlebot Facestabber
	MT_FACESTABBERSPEAR, // Castlebot Facestabber spear aura
	MT_EGGGUARD, // Egg Guard
	MT_EGGSHIELD, // Egg Guard's shield
	MT_GSNAPPER, // Green Snapper
	MT_SNAPPER_LEG, // Green Snapper leg
	MT_SNAPPER_HEAD, // Green Snapper head
	MT_MINUS, // Minus
	MT_MINUSDIRT, // Minus dirt
	MT_SPRINGSHELL, // Spring Shell
	MT_YELLOWSHELL, // Spring Shell (yellow)
	MT_UNIDUS, // Unidus
	MT_UNIBALL, // Unidus Ball
	MT_CANARIVORE, // Canarivore
	MT_CANARIVORE_GAS, // Canarivore gas
	MT_PYREFLY, // Pyre Fly
	MT_PYREFLY_FIRE, // Pyre Fly fire
	MT_PTERABYTESPAWNER, // Pterabyte spawner
	MT_PTERABYTEWAYPOINT, // Pterabyte waypoint
	MT_PTERABYTE, // Pterabyte
	MT_DRAGONBOMBER, // Dragonbomber
	MT_DRAGONWING, // Dragonbomber wing
	MT_DRAGONTAIL, // Dragonbomber tail segment
	MT_DRAGONMINE, // Dragonbomber mine

	// Generic Boss Items
	MT_BOSSEXPLODE,
	MT_SONIC3KBOSSEXPLODE,
	MT_BOSSFLYPOINT,
	MT_EGGTRAP,
	MT_BOSS3WAYPOINT,
	MT_BOSS9GATHERPOINT,
	MT_BOSSJUNK,

	// Boss 1
	MT_EGGMOBILE,
	MT_JETFUME1,
	MT_EGGMOBILE_BALL,
	MT_EGGMOBILE_TARGET,
	MT_EGGMOBILE_FIRE,

	// Boss 2
	MT_EGGMOBILE2,
	MT_EGGMOBILE2_POGO,
	MT_GOOP,
	MT_GOOPTRAIL,

	// Boss 3
	MT_EGGMOBILE3,
	MT_FAKEMOBILE,
	MT_SHOCKWAVE,

	// Boss 4
	MT_EGGMOBILE4,
	MT_EGGMOBILE4_MACE,
	MT_JETFLAME,
	MT_EGGROBO1,
	MT_EGGROBO1JET,

	// Boss 5
	MT_FANG,
	MT_BROKENROBOT,
	MT_VWREF,
	MT_VWREB,
	MT_PROJECTORLIGHT,
	MT_FBOMB,
	MT_TNTDUST, // also used by barrel
	MT_FSGNA,
	MT_FSGNB,
	MT_FANGWAYPOINT,

	// Black Eggman (Boss 7)
	MT_BLACKEGGMAN,
	MT_BLACKEGGMAN_HELPER,
	MT_BLACKEGGMAN_GOOPFIRE,
	MT_BLACKEGGMAN_MISSILE,

	// New Very-Last-Minute 2.1 Brak Eggman (Cy-Brak-demon)
	MT_CYBRAKDEMON,
	MT_CYBRAKDEMON_ELECTRIC_BARRIER,
	MT_CYBRAKDEMON_MISSILE,
	MT_CYBRAKDEMON_FLAMESHOT,
	MT_CYBRAKDEMON_FLAMEREST,
	MT_CYBRAKDEMON_TARGET_RETICULE,
	MT_CYBRAKDEMON_TARGET_DOT,
	MT_CYBRAKDEMON_NAPALM_BOMB_LARGE,
	MT_CYBRAKDEMON_NAPALM_BOMB_SMALL,
	MT_CYBRAKDEMON_NAPALM_FLAMES,
	MT_CYBRAKDEMON_VILE_EXPLOSION,

	// Metal Sonic (Boss 9)
	MT_METALSONIC_RACE,
	MT_METALSONIC_BATTLE,
	MT_MSSHIELD_FRONT,
	MT_MSGATHER,

	// Collectible Items
	MT_RING,
	MT_FLINGRING, // Lost ring
	MT_BLUESPHERE,  // Blue sphere for special stages
	MT_FLINGBLUESPHERE, // Lost blue sphere
	MT_BOMBSPHERE,
	MT_REDTEAMRING,  //Rings collectable by red team.
	MT_BLUETEAMRING, //Rings collectable by blue team.
	MT_TOKEN, // Special Stage token for special stage
	MT_REDFLAG, // Red CTF Flag
	MT_BLUEFLAG, // Blue CTF Flag
	MT_EMBLEM,
	MT_EMERALD1,
	MT_EMERALD2,
	MT_EMERALD3,
	MT_EMERALD4,
	MT_EMERALD5,
	MT_EMERALD6,
	MT_EMERALD7,
	MT_EMERHUNT, // Emerald Hunt
	MT_EMERALDSPAWN, // Emerald spawner w/ delay
	MT_FLINGEMERALD, // Lost emerald

	// Springs and others
	MT_FAN,
	MT_STEAM,
	MT_BUMPER,
	MT_BALLOON,

	MT_YELLOWSPRING,
	MT_REDSPRING,
	MT_BLUESPRING,
	MT_YELLOWDIAG,
	MT_REDDIAG,
	MT_BLUEDIAG,
	MT_YELLOWHORIZ,
	MT_REDHORIZ,
	MT_BLUEHORIZ,

	MT_BOOSTERSEG,
	MT_BOOSTERROLLER,
	MT_YELLOWBOOSTER,
	MT_REDBOOSTER,

	// Interactive Objects
	MT_BUBBLES, // Bubble source
	MT_SIGN, // Level end sign
	MT_SPIKEBALL, // Spike Ball
	MT_SPINFIRE,
	MT_SPIKE,
	MT_WALLSPIKE,
	MT_WALLSPIKEBASE,
	MT_STARPOST,
	MT_BIGMINE,
	MT_BLASTEXECUTOR,
	MT_CANNONLAUNCHER,

	// Monitor miscellany
	MT_BOXSPARKLE,

	// Monitor boxes -- regular
	MT_RING_BOX,
	MT_PITY_BOX,
	MT_ATTRACT_BOX,
	MT_FORCE_BOX,
	MT_ARMAGEDDON_BOX,
	MT_WHIRLWIND_BOX,
	MT_ELEMENTAL_BOX,
	MT_SNEAKERS_BOX,
	MT_INVULN_BOX,
	MT_1UP_BOX,
	MT_EGGMAN_BOX,
	MT_MIXUP_BOX,
	MT_MYSTERY_BOX,
	MT_GRAVITY_BOX,
	MT_RECYCLER_BOX,
	MT_SCORE1K_BOX,
	MT_SCORE10K_BOX,
	MT_FLAMEAURA_BOX,
	MT_BUBBLEWRAP_BOX,
	MT_THUNDERCOIN_BOX,

	// Monitor boxes -- repeating (big) boxes
	MT_PITY_GOLDBOX,
	MT_ATTRACT_GOLDBOX,
	MT_FORCE_GOLDBOX,
	MT_ARMAGEDDON_GOLDBOX,
	MT_WHIRLWIND_GOLDBOX,
	MT_ELEMENTAL_GOLDBOX,
	MT_SNEAKERS_GOLDBOX,
	MT_INVULN_GOLDBOX,
	MT_EGGMAN_GOLDBOX,
	MT_GRAVITY_GOLDBOX,
	MT_FLAMEAURA_GOLDBOX,
	MT_BUBBLEWRAP_GOLDBOX,
	MT_THUNDERCOIN_GOLDBOX,

	// Monitor boxes -- special
	MT_RING_REDBOX,
	MT_RING_BLUEBOX,

	// Monitor icons
	MT_RING_ICON,
	MT_PITY_ICON,
	MT_ATTRACT_ICON,
	MT_FORCE_ICON,
	MT_ARMAGEDDON_ICON,
	MT_WHIRLWIND_ICON,
	MT_ELEMENTAL_ICON,
	MT_SNEAKERS_ICON,
	MT_INVULN_ICON,
	MT_1UP_ICON,
	MT_EGGMAN_ICON,
	MT_MIXUP_ICON,
	MT_GRAVITY_ICON,
	MT_RECYCLER_ICON,
	MT_SCORE1K_ICON,
	MT_SCORE10K_ICON,
	MT_FLAMEAURA_ICON,
	MT_BUBBLEWRAP_ICON,
	MT_THUNDERCOIN_ICON,

	// Projectiles
	MT_ROCKET,
	MT_LASER,
	MT_TORPEDO,
	MT_TORPEDO2, // silent
	MT_ENERGYBALL,
	MT_MINE, // Skim/Jetty-Syn mine
	MT_JETTBULLET, // Jetty-Syn Bullet
	MT_TURRETLASER,
	MT_CANNONBALL, // Cannonball
	MT_CANNONBALLDECOR, // Decorative/still cannonball
	MT_ARROW, // Arrow
	MT_DEMONFIRE, // Glaregoyle fire

	// The letter
	MT_LETTER,

	// Tutorial Scenery
	MT_TUTORIALPLANT,
	MT_TUTORIALLEAF,
	MT_TUTORIALFLOWER,
	MT_TUTORIALFLOWERF,

	// Greenflower Scenery
	MT_GFZFLOWER1,
	MT_GFZFLOWER2,
	MT_GFZFLOWER3,

	MT_BLUEBERRYBUSH,
	MT_BERRYBUSH,
	MT_BUSH,

	// Trees (both GFZ and misc)
	MT_GFZTREE,
	MT_GFZBERRYTREE,
	MT_GFZCHERRYTREE,
	MT_CHECKERTREE,
	MT_CHECKERSUNSETTREE,
	MT_FHZTREE, // Frozen Hillside
	MT_FHZPINKTREE,
	MT_POLYGONTREE,
	MT_BUSHTREE,
	MT_BUSHREDTREE,
	MT_SPRINGTREE,

	// Techno Hill Scenery
	MT_THZFLOWER1,
	MT_THZFLOWER2,
	MT_THZFLOWER3,
	MT_THZTREE, // Steam whistle tree/bush
	MT_THZTREEBRANCH, // branch of said tree
	MT_ALARM,

	// Deep Sea Scenery
	MT_GARGOYLE, // Deep Sea Gargoyle
	MT_BIGGARGOYLE, // Deep Sea Gargoyle (Big)
	MT_SEAWEED, // DSZ Seaweed
	MT_WATERDRIP, // Dripping Water source
	MT_WATERDROP, // Water drop from dripping water
	MT_CORAL1, // Coral
	MT_CORAL2,
	MT_CORAL3,
	MT_CORAL4,
	MT_CORAL5,
	MT_BLUECRYSTAL, // Blue Crystal
	MT_KELP, // Kelp
	MT_ANIMALGAETOP, // Animated algae top
	MT_ANIMALGAESEG, // Animated algae segment
	MT_DSZSTALAGMITE, // Deep Sea 1 Stalagmite
	MT_DSZ2STALAGMITE, // Deep Sea 2 Stalagmite
	MT_LIGHTBEAM, // DSZ Light beam

	// Castle Eggman Scenery
	MT_CHAIN, // CEZ Chain
	MT_FLAME, // Flame (has corona)
	MT_FLAMEPARTICLE,
	MT_EGGSTATUE, // Eggman Statue
	MT_MACEPOINT, // Mace rotation point
	MT_CHAINMACEPOINT, // Combination of chains and maces point
	MT_SPRINGBALLPOINT, // Spring ball point
	MT_CHAINPOINT, // Mace chain
	MT_HIDDEN_SLING, // Spin mace chain (activatable)
	MT_FIREBARPOINT, // Firebar
	MT_CUSTOMMACEPOINT, // Custom mace
	MT_SMALLMACECHAIN, // Small Mace Chain
	MT_BIGMACECHAIN, // Big Mace Chain
	MT_SMALLMACE, // Small Mace
	MT_BIGMACE, // Big Mace
	MT_SMALLGRABCHAIN, // Small Grab Chain
	MT_BIGGRABCHAIN, // Big Grab Chain
	MT_BLUESPRINGBALL, // Blue spring on a ball
	MT_YELLOWSPRINGBALL, // Yellow spring on a ball
	MT_REDSPRINGBALL, // Red spring on a ball
	MT_SMALLFIREBAR, // Small Firebar
	MT_BIGFIREBAR, // Big Firebar
	MT_CEZFLOWER, // Flower
	MT_CEZPOLE1, // Pole (with red banner)
	MT_CEZPOLE2, // Pole (with blue banner)
	MT_CEZBANNER1, // Banner (red)
	MT_CEZBANNER2, // Banner (blue)
	MT_PINETREE, // Pine Tree
	MT_CEZBUSH1, // Bush 1
	MT_CEZBUSH2, // Bush 2
	MT_CANDLE, // Candle
	MT_CANDLEPRICKET, // Candle pricket
	MT_FLAMEHOLDER, // Flame holder
	MT_FIRETORCH, // Fire torch
	MT_WAVINGFLAG1, // Waving flag (red)
	MT_WAVINGFLAG2, // Waving flag (blue)
	MT_WAVINGFLAGSEG1, // Waving flag segment (red)
	MT_WAVINGFLAGSEG2, // Waving flag segment (blue)
	MT_CRAWLASTATUE, // Crawla statue
	MT_FACESTABBERSTATUE, // Facestabber statue
	MT_SUSPICIOUSFACESTABBERSTATUE, // :eggthinking:
	MT_BRAMBLES, // Brambles

	// Arid Canyon Scenery
	MT_BIGTUMBLEWEED,
	MT_LITTLETUMBLEWEED,
	MT_CACTI1, // Tiny Red Flower Cactus
	MT_CACTI2, // Small Red Flower Cactus
	MT_CACTI3, // Tiny Blue Flower Cactus
	MT_CACTI4, // Small Blue Flower Cactus
	MT_CACTI5, // Prickly Pear
	MT_CACTI6, // Barrel Cactus
	MT_CACTI7, // Tall Barrel Cactus
	MT_CACTI8, // Armed Cactus
	MT_CACTI9, // Ball Cactus
	MT_CACTI10, // Tiny Cactus
	MT_CACTI11, // Small Cactus
	MT_CACTITINYSEG, // Tiny Cactus Segment
	MT_CACTISMALLSEG, // Small Cactus Segment
	MT_ARIDSIGN_CAUTION, // Caution Sign
	MT_ARIDSIGN_CACTI, // Cacti Sign
	MT_ARIDSIGN_SHARPTURN, // Sharp Turn Sign
	MT_OILLAMP,
	MT_TNTBARREL,
	MT_PROXIMITYTNT,
	MT_DUSTDEVIL,
	MT_DUSTLAYER,
	MT_ARIDDUST,
	MT_MINECART,
	MT_MINECARTSEG,
	MT_MINECARTSPAWNER,
	MT_MINECARTEND,
	MT_MINECARTENDSOLID,
	MT_MINECARTSIDEMARK,
	MT_MINECARTSPARK,
	MT_SALOONDOOR,
	MT_SALOONDOORCENTER,
	MT_TRAINCAMEOSPAWNER,
	MT_TRAINSEG,
	MT_TRAINDUSTSPAWNER,
	MT_TRAINSTEAMSPAWNER,
	MT_MINECARTSWITCHPOINT,

	// Red Volcano Scenery
	MT_FLAMEJET,
	MT_VERTICALFLAMEJET,
	MT_FLAMEJETFLAME,

	MT_FJSPINAXISA, // Counter-clockwise
	MT_FJSPINAXISB, // Clockwise

	MT_FLAMEJETFLAMEB, // Blade's flame

	MT_LAVAFALL,
	MT_LAVAFALL_LAVA,
	MT_LAVAFALLROCK,

	MT_ROLLOUTSPAWN,
	MT_ROLLOUTROCK,

	MT_BIGFERNLEAF,
	MT_BIGFERN,
	MT_JUNGLEPALM,
	MT_TORCHFLOWER,
	MT_WALLVINE_LONG,
	MT_WALLVINE_SHORT,

	// Dark City Scenery

	// Egg Rock Scenery

	// Azure Temple Scenery
	MT_GLAREGOYLE,
	MT_GLAREGOYLEUP,
	MT_GLAREGOYLEDOWN,
	MT_GLAREGOYLELONG,
	MT_TARGET, // AKA Red Crystal
	MT_GREENFLAME,
	MT_BLUEGARGOYLE,

	// Stalagmites
	MT_STALAGMITE0,
	MT_STALAGMITE1,
	MT_STALAGMITE2,
	MT_STALAGMITE3,
	MT_STALAGMITE4,
	MT_STALAGMITE5,
	MT_STALAGMITE6,
	MT_STALAGMITE7,
	MT_STALAGMITE8,
	MT_STALAGMITE9,

	// Christmas Scenery
	MT_XMASPOLE,
	MT_CANDYCANE,
	MT_SNOWMAN,    // normal
	MT_SNOWMANHAT, // with hat + scarf
	MT_LAMPPOST1,  // normal
	MT_LAMPPOST2,  // with snow
	MT_HANGSTAR,
	MT_MISTLETOE,
	MT_SSZTREE,
	MT_SSZTREE_BRANCH,
	MT_SSZTREE2,
	MT_SSZTREE2_BRANCH,
	// Xmas GFZ bushes
	MT_XMASBLUEBERRYBUSH,
	MT_XMASBERRYBUSH,
	MT_XMASBUSH,
	// FHZ
	MT_FHZICE1,
	MT_FHZICE2,
	MT_ROSY,
	MT_CDLHRT,

	// Halloween Scenery
	// Pumpkins
	MT_JACKO1,
	MT_JACKO2,
	MT_JACKO3,
	// Dr Seuss Trees
	MT_HHZTREE_TOP,
	MT_HHZTREE_PART,
	// Misc
	MT_HHZSHROOM,
	MT_HHZGRASS,
	MT_HHZTENTACLE1,
	MT_HHZTENTACLE2,
	MT_HHZSTALAGMITE_TALL,
	MT_HHZSTALAGMITE_SHORT,

	// Botanic Serenity scenery
	MT_BSZTALLFLOWER_RED,
	MT_BSZTALLFLOWER_PURPLE,
	MT_BSZTALLFLOWER_BLUE,
	MT_BSZTALLFLOWER_CYAN,
	MT_BSZTALLFLOWER_YELLOW,
	MT_BSZTALLFLOWER_ORANGE,
	MT_BSZFLOWER_RED,
	MT_BSZFLOWER_PURPLE,
	MT_BSZFLOWER_BLUE,
	MT_BSZFLOWER_CYAN,
	MT_BSZFLOWER_YELLOW,
	MT_BSZFLOWER_ORANGE,
	MT_BSZSHORTFLOWER_RED,
	MT_BSZSHORTFLOWER_PURPLE,
	MT_BSZSHORTFLOWER_BLUE,
	MT_BSZSHORTFLOWER_CYAN,
	MT_BSZSHORTFLOWER_YELLOW,
	MT_BSZSHORTFLOWER_ORANGE,
	MT_BSZTULIP_RED,
	MT_BSZTULIP_PURPLE,
	MT_BSZTULIP_BLUE,
	MT_BSZTULIP_CYAN,
	MT_BSZTULIP_YELLOW,
	MT_BSZTULIP_ORANGE,
	MT_BSZCLUSTER_RED,
	MT_BSZCLUSTER_PURPLE,
	MT_BSZCLUSTER_BLUE,
	MT_BSZCLUSTER_CYAN,
	MT_BSZCLUSTER_YELLOW,
	MT_BSZCLUSTER_ORANGE,
	MT_BSZBUSH_RED,
	MT_BSZBUSH_PURPLE,
	MT_BSZBUSH_BLUE,
	MT_BSZBUSH_CYAN,
	MT_BSZBUSH_YELLOW,
	MT_BSZBUSH_ORANGE,
	MT_BSZVINE_RED,
	MT_BSZVINE_PURPLE,
	MT_BSZVINE_BLUE,
	MT_BSZVINE_CYAN,
	MT_BSZVINE_YELLOW,
	MT_BSZVINE_ORANGE,
	MT_BSZSHRUB,
	MT_BSZCLOVER,
	MT_BIG_PALMTREE_TRUNK,
	MT_BIG_PALMTREE_TOP,
	MT_PALMTREE_TRUNK,
	MT_PALMTREE_TOP,

	// Misc scenery
	MT_DBALL,
	MT_EGGSTATUE2,
	MT_GINE,
	MT_PPAL,
	MT_PPEL,

	// Powerup Indicators
	MT_ELEMENTAL_ORB, // Elemental shield mobj
	MT_ATTRACT_ORB, // Attract shield mobj
	MT_FORCE_ORB, // Force shield mobj
	MT_ARMAGEDDON_ORB, // Armageddon shield mobj
	MT_WHIRLWIND_ORB, // Whirlwind shield mobj
	MT_PITY_ORB, // Pity shield mobj
	MT_FLAMEAURA_ORB, // Flame shield mobj
	MT_BUBBLEWRAP_ORB, // Bubble shield mobj
	MT_THUNDERCOIN_ORB, // Thunder shield mobj
	MT_THUNDERCOIN_SPARK, // Thunder spark
	MT_IVSP, // Invincibility sparkles
	MT_SUPERSPARK, // Super Sonic Spark

	// Flickies
	MT_FLICKY_01, // Bluebird
	MT_FLICKY_01_CENTER,
	MT_FLICKY_02, // Rabbit
	MT_FLICKY_02_CENTER,
	MT_FLICKY_03, // Chicken
	MT_FLICKY_03_CENTER,
	MT_FLICKY_04, // Seal
	MT_FLICKY_04_CENTER,
	MT_FLICKY_05, // Pig
	MT_FLICKY_05_CENTER,
	MT_FLICKY_06, // Chipmunk
	MT_FLICKY_06_CENTER,
	MT_FLICKY_07, // Penguin
	MT_FLICKY_07_CENTER,
	MT_FLICKY_08, // Fish
	MT_FLICKY_08_CENTER,
	MT_FLICKY_09, // Ram
	MT_FLICKY_09_CENTER,
	MT_FLICKY_10, // Puffin
	MT_FLICKY_10_CENTER,
	MT_FLICKY_11, // Cow
	MT_FLICKY_11_CENTER,
	MT_FLICKY_12, // Rat
	MT_FLICKY_12_CENTER,
	MT_FLICKY_13, // Bear
	MT_FLICKY_13_CENTER,
	MT_FLICKY_14, // Dove
	MT_FLICKY_14_CENTER,
	MT_FLICKY_15, // Cat
	MT_FLICKY_15_CENTER,
	MT_FLICKY_16, // Canary
	MT_FLICKY_16_CENTER,
	MT_SECRETFLICKY_01, // Spider
	MT_SECRETFLICKY_01_CENTER,
	MT_SECRETFLICKY_02, // Bat
	MT_SECRETFLICKY_02_CENTER,
	MT_SEED,

	// Environmental Effects
	MT_RAIN, // Rain
	MT_SNOWFLAKE, // Snowflake
	MT_SPLISH, // Water splish!
	MT_LAVASPLISH, // Lava splish!
	MT_SMOKE,
	MT_SMALLBUBBLE, // small bubble
	MT_MEDIUMBUBBLE, // medium bubble
	MT_EXTRALARGEBUBBLE, // extra large bubble
	MT_WATERZAP,
	MT_SPINDUST, // Spindash dust
	MT_TFOG,
	MT_PARTICLE,
	MT_PARTICLEGEN, // For fans, etc.

	// Game Indicators
	MT_SCORE, // score logo
	MT_DROWNNUMBERS, // Drowning Timer
	MT_GOTEMERALD, // Chaos Emerald (intangible)
	MT_LOCKON, // Target
	MT_LOCKONINF, // In-level Target
	MT_TAG, // Tag Sign
	MT_GOTFLAG, // Got Flag sign
	MT_FINISHFLAG, // Finish flag

	// Ambient Sounds
	MT_AMBIENT,

	MT_CORK,
	MT_LHRT,

	// Ring Weapons
	MT_REDRING,
	MT_BOUNCERING,
	MT_RAILRING,
	MT_INFINITYRING,
	MT_AUTOMATICRING,
	MT_EXPLOSIONRING,
	MT_SCATTERRING,
	MT_GRENADERING,

	MT_BOUNCEPICKUP,
	MT_RAILPICKUP,
	MT_AUTOPICKUP,
	MT_EXPLODEPICKUP,
	MT_SCATTERPICKUP,
	MT_GRENADEPICKUP,

	MT_THROWNBOUNCE,
	MT_THROWNINFINITY,
	MT_THROWNAUTOMATIC,
	MT_THROWNSCATTER,
	MT_THROWNEXPLOSION,
	MT_THROWNGRENADE,

	// Mario-specific stuff
	MT_COIN,
	MT_FLINGCOIN,
	MT_GOOMBA,
	MT_BLUEGOOMBA,
	MT_FIREFLOWER,
	MT_FIREBALL,
	MT_FIREBALLTRAIL,
	MT_SHELL,
	MT_PUMA,
	MT_PUMATRAIL,
	MT_HAMMER,
	MT_KOOPA,
	MT_KOOPAFLAME,
	MT_AXE,
	MT_MARIOBUSH1,
	MT_MARIOBUSH2,
	MT_TOAD,

	// NiGHTS Stuff
	MT_AXIS,
	MT_AXISTRANSFER,
	MT_AXISTRANSFERLINE,
	MT_NIGHTSDRONE,
	MT_NIGHTSDRONE_MAN,
	MT_NIGHTSDRONE_SPARKLING,
	MT_NIGHTSDRONE_GOAL,
	MT_NIGHTSPARKLE,
	MT_NIGHTSLOOPHELPER,
	MT_NIGHTSBUMPER, // NiGHTS Bumper
	MT_HOOP,
	MT_HOOPCOLLIDE, // Collision detection for NiGHTS hoops
	MT_HOOPCENTER, // Center of a hoop
	MT_NIGHTSCORE,
	MT_NIGHTSCHIP, // NiGHTS Chip
	MT_FLINGNIGHTSCHIP, // Lost NiGHTS Chip
	MT_NIGHTSSTAR, // NiGHTS Star
	MT_FLINGNIGHTSSTAR, // Lost NiGHTS Star
	MT_NIGHTSSUPERLOOP,
	MT_NIGHTSDRILLREFILL,
	MT_NIGHTSHELPER,
	MT_NIGHTSEXTRATIME,
	MT_NIGHTSLINKFREEZE,
	MT_EGGCAPSULE,
	MT_IDEYAANCHOR,
	MT_NIGHTOPIANHELPER, // the actual helper object that orbits you
	MT_PIAN, // decorative singing friend
	MT_SHLEEP, // almost-decorative sleeping enemy

	// Secret badniks and hazards, shhhh
	MT_PENGUINATOR,
	MT_POPHAT,
	MT_POPSHOT,
	MT_POPSHOT_TRAIL,

	MT_HIVEELEMENTAL,
	MT_BUMBLEBORE,

	MT_BUGGLE,

	MT_SMASHINGSPIKEBALL,
	MT_CACOLANTERN,
	MT_CACOSHARD,
	MT_CACOFIRE,
	MT_SPINBOBERT,
	MT_SPINBOBERT_FIRE1,
	MT_SPINBOBERT_FIRE2,
	MT_HANGSTER,

	// Utility Objects
	MT_TELEPORTMAN,
	MT_ALTVIEWMAN,
	MT_CRUMBLEOBJ, // Sound generator for crumbling platform
	MT_TUBEWAYPOINT,
	MT_PUSH,
	MT_GHOST,
	MT_OVERLAY,
	MT_ANGLEMAN,
	MT_POLYANCHOR,
	MT_POLYSPAWN,

	// Portal objects
	MT_SKYBOX,

	// Debris
	MT_SPARK, //spark
	MT_EXPLODE, // Robot Explosion
	MT_UWEXPLODE, // Underwater Explosion
	MT_DUST,
	MT_ROCKSPAWNER,
	MT_FALLINGROCK,
	MT_ROCKCRUMBLE1,
	MT_ROCKCRUMBLE2,
	MT_ROCKCRUMBLE3,
	MT_ROCKCRUMBLE4,
	MT_ROCKCRUMBLE5,
	MT_ROCKCRUMBLE6,
	MT_ROCKCRUMBLE7,
	MT_ROCKCRUMBLE8,
	MT_ROCKCRUMBLE9,
	MT_ROCKCRUMBLE10,
	MT_ROCKCRUMBLE11,
	MT_ROCKCRUMBLE12,
	MT_ROCKCRUMBLE13,
	MT_ROCKCRUMBLE14,
	MT_ROCKCRUMBLE15,
	MT_ROCKCRUMBLE16,

	// Level debris
	MT_GFZDEBRIS,
	MT_BRICKDEBRIS,
	MT_WOODDEBRIS,
	MT_REDBRICKDEBRIS, // for CEZ3
	MT_BLUEBRICKDEBRIS, // for CEZ3
	MT_YELLOWBRICKDEBRIS, // for CEZ3

	MT_NAMECHECK,
	MT_RAY, // General purpose mobj

	MT_OLDK,
};

typedef INT32 mobjtype_t;

typedef struct
{
	INT32 doomednum;
	statenum_t spawnstate;
	INT32 spawnhealth;
	statenum_t seestate;
	sfxenum_t seesound;
	INT32 reactiontime;
	sfxenum_t attacksound;
	statenum_t painstate;
	INT32 painchance;
	sfxenum_t painsound;
	statenum_t meleestate;
	statenum_t missilestate;
	statenum_t deathstate;
	statenum_t xdeathstate;
	sfxenum_t deathsound;
	fixed_t speed;
	fixed_t radius;
	fixed_t height;
	INT32 dispoffset;
	INT32 mass;
	INT32 damage;
	sfxenum_t activesound;
	UINT32 flags;
	statenum_t raisestate;
} mobjinfo_t;

extern mobjinfo_t mobjinfo[NUMMOBJTYPES];

extern statenum_t S_ALART1;
extern statenum_t S_BLACKEGG_DESTROYPLAT1;
extern statenum_t S_BLACKEGG_DIE4;
extern statenum_t S_BLACKEGG_GOOP;
extern statenum_t S_BLACKEGG_HITFACE4;
extern statenum_t S_BLACKEGG_JUMP1;
extern statenum_t S_BLACKEGG_JUMP2;
extern statenum_t S_BLACKEGG_PAIN1;
extern statenum_t S_BLACKEGG_PAIN35;
extern statenum_t S_BLACKEGG_SHOOT1;
extern statenum_t S_BLACKEGG_SHOOT2;
extern statenum_t S_BLACKEGG_STND;
extern statenum_t S_BLACKEGG_WALK1;
extern statenum_t S_BLACKEGG_WALK6;
extern statenum_t S_BLUESPHEREBONUS;
extern statenum_t S_BOSSEGLZ1;
extern statenum_t S_BOSSEGLZ2;
extern statenum_t S_BOSSSEBH1;
extern statenum_t S_BOSSSEBH2;
extern statenum_t S_BOSSSPIGOT;
extern statenum_t S_BOSSTANK1;
extern statenum_t S_BOSSTANK2;
extern statenum_t S_BUMBLEBORE_FALL2;
extern statenum_t S_BUMBLEBORE_FLY1;
extern statenum_t S_BUMBLEBORE_FLY2;
extern statenum_t S_BUMBLEBORE_RAISE;
extern statenum_t S_BUMBLEBORE_STUCK2;
extern statenum_t S_CEMG1;
extern statenum_t S_CEMG2;
extern statenum_t S_CEMG3;
extern statenum_t S_CEMG4;
extern statenum_t S_CEMG5;
extern statenum_t S_CEMG6;
extern statenum_t S_CEMG7;
extern statenum_t S_CLEARSIGN;
extern statenum_t S_CRUSHCLAW_STAY;
extern statenum_t S_CYBRAKDEMON_DIE8;
extern statenum_t S_EGGMANSIGN;
extern statenum_t S_EXTRALARGEBUBBLE;
extern statenum_t S_FANG_BOUNCE3;
extern statenum_t S_FANG_BOUNCE4;
extern statenum_t S_FANG_INTRO12;
extern statenum_t S_FANG_PAIN1;
extern statenum_t S_FANG_PAIN2;
extern statenum_t S_FANG_PATHINGCONT1;
extern statenum_t S_FANG_PATHINGCONT2;
extern statenum_t S_FANG_PINCHBOUNCE3;
extern statenum_t S_FANG_PINCHBOUNCE4;
extern statenum_t S_FANG_PINCHPATHINGSTART2;
extern statenum_t S_FANG_WALLHIT;
extern statenum_t S_FLAMEJETFLAME4;
extern statenum_t S_FLAMEJETFLAME7;
extern statenum_t S_FLIGHTINDICATOR;
extern statenum_t S_HANGSTER_ARC1;
extern statenum_t S_HANGSTER_ARC2;
extern statenum_t S_HANGSTER_ARC3;
extern statenum_t S_HANGSTER_ARCUP1;
extern statenum_t S_HANGSTER_FLY1;
extern statenum_t S_HANGSTER_RETURN1;
extern statenum_t S_HANGSTER_RETURN2;
extern statenum_t S_HANGSTER_RETURN3;
extern statenum_t S_HANGSTER_SWOOP1;
extern statenum_t S_HANGSTER_SWOOP2;
extern statenum_t S_INVISIBLE;
extern statenum_t S_JETFUME1;
extern statenum_t S_JETFUMEFLASH;
extern statenum_t S_LAVAFALL_DORMANT;
extern statenum_t S_LAVAFALL_SHOOT;
extern statenum_t S_LAVAFALL_TELL;
extern statenum_t S_METALSONIC_DASH;
extern statenum_t S_METALSONIC_BOUNCE;
extern statenum_t S_MINECARTSEG_FRONT;
extern statenum_t S_MSSHIELD_F2;
extern statenum_t S_NIGHTSDRONE_SPARKLING1;
extern statenum_t S_NIGHTSDRONE_SPARKLING16;
extern statenum_t S_NULL;
extern statenum_t S_OBJPLACE_DUMMY;
extern statenum_t S_OILLAMPFLARE;
extern statenum_t S_PLAY_BOUNCE;
extern statenum_t S_PLAY_BOUNCE_LANDING;
extern statenum_t S_PLAY_CLIMB;
extern statenum_t S_PLAY_CLING;
extern statenum_t S_PLAY_DASH;
extern statenum_t S_PLAY_DEAD;
extern statenum_t S_PLAY_DRWN;
extern statenum_t S_PLAY_EDGE;
extern statenum_t S_PLAY_FALL;
extern statenum_t S_PLAY_FIRE;
extern statenum_t S_PLAY_FIRE_FINISH;
extern statenum_t S_PLAY_FLOAT;
extern statenum_t S_PLAY_FLOAT_RUN;
extern statenum_t S_PLAY_FLY;
extern statenum_t S_PLAY_FLY_TIRED;
extern statenum_t S_PLAY_GASP;
extern statenum_t S_PLAY_GLIDE;
extern statenum_t S_PLAY_GLIDE_LANDING;
extern statenum_t S_PLAY_JUMP;
extern statenum_t S_PLAY_MELEE;
extern statenum_t S_PLAY_MELEE_FINISH;
extern statenum_t S_PLAY_MELEE_LANDING;
extern statenum_t S_PLAY_NIGHTS_ATTACK;
extern statenum_t S_PLAY_NIGHTS_DRILL;
extern statenum_t S_PLAY_NIGHTS_FLOAT;
extern statenum_t S_PLAY_NIGHTS_FLY;
extern statenum_t S_PLAY_NIGHTS_PULL;
extern statenum_t S_PLAY_NIGHTS_STAND;
extern statenum_t S_PLAY_NIGHTS_STUN;
extern statenum_t S_PLAY_NIGHTS_TRANS1;
extern statenum_t S_PLAY_NIGHTS_TRANS6;
extern statenum_t S_PLAY_PAIN;
extern statenum_t S_PLAY_RIDE;
extern statenum_t S_PLAY_ROLL;
extern statenum_t S_PLAY_RUN;
extern statenum_t S_PLAY_SIGN;
extern statenum_t S_PLAY_SKID;
extern statenum_t S_PLAY_SPINDASH;
extern statenum_t S_PLAY_SPRING;
extern statenum_t S_PLAY_STND;
extern statenum_t S_PLAY_STUN;
extern statenum_t S_PLAY_SUPER_TRANS1;
extern statenum_t S_PLAY_SUPER_TRANS6;
extern statenum_t S_PLAY_SWIM;
extern statenum_t S_PLAY_TWINSPIN;
extern statenum_t S_PLAY_WAIT;
extern statenum_t S_PLAY_WALK;
extern statenum_t S_PTERABYTE_FLY1;
extern statenum_t S_PTERABYTE_SWOOPDOWN;
extern statenum_t S_PTERABYTE_SWOOPUP;
extern statenum_t S_RAIN1;
extern statenum_t S_RAINRETURN;
extern statenum_t S_REDBOOSTERROLLER;
extern statenum_t S_REDBOOSTERSEG_FACE;
extern statenum_t S_REDBOOSTERSEG_LEFT;
extern statenum_t S_REDBOOSTERSEG_RIGHT;
extern statenum_t S_ROSY_HUG;
extern statenum_t S_ROSY_IDLE;
extern statenum_t S_ROSY_JUMP;
extern statenum_t S_ROSY_FALL;
extern statenum_t S_ROSY_PAIN;
extern statenum_t S_ROSY_STND;
extern statenum_t S_ROSY_UNHAPPY;
extern statenum_t S_ROSY_WALK;
extern statenum_t S_SIGNBOARD;
extern statenum_t S_SIGNSPIN1;
extern statenum_t S_SMASHSPIKE_FALL;
extern statenum_t S_SMASHSPIKE_FLOAT;
extern statenum_t S_SMASHSPIKE_RISE2;
extern statenum_t S_SMASHSPIKE_STOMP1;
extern statenum_t S_SNOW2;
extern statenum_t S_SNOW3;
extern statenum_t S_SPINCUSHION_AIM1;
extern statenum_t S_SPINCUSHION_AIM5;
extern statenum_t S_SPINDUST_BUBBLE1;
extern statenum_t S_SPINDUST_BUBBLE4;
extern statenum_t S_SPINDUST_FIRE1;
extern statenum_t S_SPLASH1;
extern statenum_t S_STEAM1;
extern statenum_t S_TAILSOVERLAY_0DEGREES;
extern statenum_t S_TAILSOVERLAY_DASH;
extern statenum_t S_TAILSOVERLAY_EDGE;
extern statenum_t S_TAILSOVERLAY_FLY;
extern statenum_t S_TAILSOVERLAY_GASP;
extern statenum_t S_TAILSOVERLAY_MINUS30DEGREES;
extern statenum_t S_TAILSOVERLAY_MINUS60DEGREES;
extern statenum_t S_TAILSOVERLAY_PAIN;
extern statenum_t S_TAILSOVERLAY_PLUS30DEGREES;
extern statenum_t S_TAILSOVERLAY_PLUS60DEGREES;
extern statenum_t S_TAILSOVERLAY_RUN;
extern statenum_t S_TAILSOVERLAY_STAND;
extern statenum_t S_TAILSOVERLAY_TIRE;
extern statenum_t S_TEAM_SPINFIRE1;
extern statenum_t S_TRAINDUST;
extern statenum_t S_TRAINPUFFMAKER;
extern statenum_t S_TRAINSTEAM;
extern statenum_t S_TUTORIALFLOWER1;
extern statenum_t S_TUTORIALFLOWERF1;
extern statenum_t S_TUTORIALLEAF1;
extern statenum_t S_UNKNOWN;
extern statenum_t S_XPLD_EGGTRAP;
extern statenum_t S_XPLD1;
extern statenum_t S_YELLOWBOOSTERROLLER;
extern statenum_t S_YELLOWBOOSTERSEG_FACE;
extern statenum_t S_YELLOWBOOSTERSEG_LEFT;
extern statenum_t S_YELLOWBOOSTERSEG_RIGHT;

mobjtype_t GetMobjTypeByName(const char *name);
statenum_t GetStateByName(const char *name);
void CacheInfoConstants(void);

void P_PatchInfoTables(void);

void P_BackupTables(void);

void P_ResetData(INT32 flags);

#endif
