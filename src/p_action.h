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
/// \file  p_action.h
/// \brief Action Pointer Functions that are associated with states/frames

#ifndef __P_ACTION__
#define __P_ACTION__

#include "p_mobj.h"

INT32 Action_ValueToInteger(action_val_t value);

char *Action_ValueToString(action_val_t value);

void Action_MakeString(action_string_t *out, const char *str);

// IMPORTANT NOTE: If you add/remove from this list of action
// functions, don't forget to update them in deh_tables.c!
void A_Explode(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Pain(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Fall(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MonitorPop(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_GoldMonitorPop(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_GoldMonitorRestore(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_GoldMonitorSparkle(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Look(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Chase(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FaceStabChase(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FaceStabRev(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FaceStabHurl(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FaceStabMiss(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_StatueBurst(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FaceTarget(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FaceTracer(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Scream(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_BossDeath(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SetShadowScale(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ShadowScream(mobj_t *actor, action_val_t *args, unsigned argcount); // MARIA!!!!!!
void A_CustomPower(mobj_t *actor, action_val_t *args, unsigned argcount); // Use this for a custom power
void A_GiveWeapon(mobj_t *actor, action_val_t *args, unsigned argcount); // Gives the player weapon(s)
void A_RingBox(mobj_t *actor, action_val_t *args, unsigned argcount); // Obtained Ring Box Tails
void A_Invincibility(mobj_t *actor, action_val_t *args, unsigned argcount); // Obtained Invincibility Box
void A_SuperSneakers(mobj_t *actor, action_val_t *args, unsigned argcount); // Obtained Super Sneakers Box
void A_BunnyHop(mobj_t *actor, action_val_t *args, unsigned argcount); // have bunny hop tails
void A_BubbleSpawn(mobj_t *actor, action_val_t *args, unsigned argcount); // Randomly spawn bubbles
void A_FanBubbleSpawn(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_BubbleRise(mobj_t *actor, action_val_t *args, unsigned argcount); // Bubbles float to surface
void A_BubbleCheck(mobj_t *actor, action_val_t *args, unsigned argcount); // Don't draw if not underwater
void A_AwardScore(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ExtraLife(mobj_t *actor, action_val_t *args, unsigned argcount); // Extra Life
void A_GiveShield(mobj_t *actor, action_val_t *args, unsigned argcount); // Obtained Shield
void A_GravityBox(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ScoreRise(mobj_t *actor, action_val_t *args, unsigned argcount); // Rise the score logo
void A_AttractChase(mobj_t *actor, action_val_t *args, unsigned argcount); // Ring Chase
void A_DropMine(mobj_t *actor, action_val_t *args, unsigned argcount); // Drop Mine from Skim or Jetty-Syn Bomber
void A_FishJump(mobj_t *actor, action_val_t *args, unsigned argcount); // Fish Jump
void A_ThrownRing(mobj_t *actor, action_val_t *args, unsigned argcount); // Sparkle trail for red ring
void A_SetSolidSteam(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_UnsetSolidSteam(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SignSpin(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SignPlayer(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_OverlayThink(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_JetChase(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_JetbThink(mobj_t *actor, action_val_t *args, unsigned argcount); // Jetty-Syn Bomber Thinker
void A_JetgThink(mobj_t *actor, action_val_t *args, unsigned argcount); // Jetty-Syn Gunner Thinker
void A_JetgShoot(mobj_t *actor, action_val_t *args, unsigned argcount); // Jetty-Syn Shoot Function
void A_ShootBullet(mobj_t *actor, action_val_t *args, unsigned argcount); // JetgShoot without reactiontime setting
void A_MinusDigging(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MinusPopup(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MinusCheck(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ChickenCheck(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MouseThink(mobj_t *actor, action_val_t *args, unsigned argcount); // Mouse Thinker
void A_DetonChase(mobj_t *actor, action_val_t *args, unsigned argcount); // Deton Chaser
void A_CapeChase(mobj_t *actor, action_val_t *args, unsigned argcount); // Fake little Super Sonic cape
void A_RotateSpikeBall(mobj_t *actor, action_val_t *args, unsigned argcount); // Spike ball rotation
void A_SlingAppear(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_UnidusBall(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_RockSpawn(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SetFuse(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CrawlaCommanderThink(mobj_t *actor, action_val_t *args, unsigned argcount); // Crawla Commander
void A_SmokeTrailer(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_RingExplode(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_OldRingExplode(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MixUp(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_RecyclePowers(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_BossScream(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss2TakeDamage(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_GoopSplat(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss2PogoSFX(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss2PogoTarget(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_EggmanBox(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_TurretFire(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SuperTurretFire(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_TurretStop(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_JetJawRoam(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_JetJawChomp(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_PointyThink(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckBuddy(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_HoodFire(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_HoodThink(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_HoodFall(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ArrowBonks(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SnailerThink(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SharpChase(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SharpSpin(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SharpDecel(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CrushstaceanWalk(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CrushstaceanPunch(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CrushclawAim(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CrushclawLaunch(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_VultureVtol(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_VultureCheck(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_VultureHover(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_VultureBlast(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_VultureFly(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SkimChase(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SkullAttack(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_LobShot(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FireShot(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SuperFireShot(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_BossFireShot(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss7FireMissiles(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss1Laser(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FocusTarget(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss4Reverse(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss4SpeedUp(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss4Raise(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SparkFollow(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_BuzzFly(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_GuardChase(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_EggShield(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SetReactionTime(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss1Spikeballs(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss3TakeDamage(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss3Path(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss3ShockThink(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Shockwave(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_LinedefExecute(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_LinedefExecuteFromArg(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_PlaySeeSound(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_PlayAttackSound(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_PlayActiveSound(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_1upThinker(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_BossZoom(mobj_t *actor, action_val_t *args, unsigned argcount); //Unused
void A_Boss1Chase(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss2Chase(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss2Pogo(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss7Chase(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_BossJetFume(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SpawnObjectAbsolute(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SpawnObjectRelative(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ChangeAngleRelative(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ChangeAngleAbsolute(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_RollAngle(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ChangeRollAngleRelative(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ChangeRollAngleAbsolute(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_PlaySound(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FindTarget(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FindTracer(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SetTics(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SetRandomTics(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ChangeColorRelative(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ChangeColorAbsolute(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Dye(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MoveRelative(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MoveAbsolute(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Thrust(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ZThrust(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SetTargetsTarget(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SetObjectFlags(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SetObjectFlags2(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_RandomState(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_RandomStateRange(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_StateRangeByAngle(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_StateRangeByParameter(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_DualAction(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_RemoteAction(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ToggleFlameJet(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_OrbitNights(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_GhostMe(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SetObjectState(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SetObjectTypeState(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_KnockBack(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_PushAway(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_RingDrain(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SplitShot(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MissileSplit(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MultiShot(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_InstaLoop(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Custom3DRotate(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SearchForPlayers(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckRandom(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckTargetRings(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckRings(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckTotalRings(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckHealth(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckRange(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckHeight(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckTrueRange(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckThingCount(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckAmbush(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckCustomValue(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckCusValMemo(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SetCustomValue(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_UseCusValMemo(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_RelayCustomValue(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CusValAction(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ForceStop(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ForceWin(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SpikeRetract(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_InfoState(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Repeat(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SetScale(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_RemoteDamage(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_HomingChase(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_TrapShot(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_VileTarget(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_VileAttack(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_VileFire(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_BrakChase(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_BrakFireShot(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_BrakLobShot(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_NapalmScatter(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SpawnFreshCopy(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FlickySpawn(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FlickyCenter(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FlickyAim(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FlickyFly(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FlickySoar(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FlickyCoast(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FlickyHop(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FlickyFlounder(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FlickyCheck(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FlickyHeightCheck(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FlickyFlutter(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FlameParticle(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FadeOverlay(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss5Jump(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_LightBeamReset(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MineExplode(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MineRange(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ConnectToGround(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SpawnParticleRelative(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MultiShotDist(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_WhoCaresIfYourSonIsABee(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ParentTriesToSleep(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CryingToMomma(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CheckFlags2(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss5FindWaypoint(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_DoNPCSkid(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_DoNPCPain(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_PrepareRepeat(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss5ExtraRepeat(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss5Calm(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss5CheckOnGround(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss5CheckFalling(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss5PinchShot(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss5MakeItRain(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss5MakeJunk(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_LookForBetter(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_Boss5BombExplode(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_DustDevilThink(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_TNTExplode(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_DebrisRandom(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_TrainCameo(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_TrainCameo2(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_CanarivoreGas(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_KillSegments(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SnapperSpawn(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SnapperThinker(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SaloonDoorSpawn(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_MinecartSparkThink(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ModuloToState(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_LavafallRocks(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_LavafallLava(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FallingLavaCheck(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_FireShrink(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_SpawnPterabytes(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_PterabyteHover(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_RolloutSpawn(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_RolloutRock(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_DragonbomberSpawn(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_DragonWing(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_DragonSegment(mobj_t *actor, action_val_t *args, unsigned argcount);
void A_ChangeHeight(mobj_t *actor, action_val_t *args, unsigned argcount);

#endif
