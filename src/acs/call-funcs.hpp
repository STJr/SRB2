// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Russell's Smart Interfaces
// Copyright (C) 2024 by Sonic Team Junior.
// Copyright (C) 2016 by James Haley, David Hill, et al. (Team Eternity)
// Copyright (C) 2024 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  call-funcs.hpp
/// \brief Action Code Script: CallFunc instructions

#ifndef __SRB2_ACS_CALL_FUNCS_HPP__
#define __SRB2_ACS_CALL_FUNCS_HPP__

#include "acsvm.hpp"

/*--------------------------------------------------
	bool CallFunc_???(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

		These are the actual CallFuncs ran when ACS
		is executed. Which CallFuncs are executed
		is based on the indices from the compiled
		data. ACS_EnvConstruct is where the link
		between the byte code and the actual function
		is made.

	Input Arguments:-
		thread: The ACS execution thread this action
			is running on.
		argV: An array of the action's arguments.
		argC: The length of the argument array.

	Return:-
		Returns true if this function pauses the
		thread's execution. Otherwise returns false.
--------------------------------------------------*/

bool CallFunc_Random(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ThingCount(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_TagWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PolyWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_CameraWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ChangeFloor(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ChangeCeiling(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_LineSide(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ClearLineSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_EndPrint(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerCount(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GameType(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Timer(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_IsNetworkGame(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SectorSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_AmbientSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetLineTexture(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetLineSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetLineRenderStyle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ChangeSky(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ThingSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_EndPrintBold(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerTeam(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerRings(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerScore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerSuper(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerNumber(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ActivatorTID(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_EndLog(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_strcmp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_strcasecmp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_CountEnemies(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_CountPushables(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_HasUnlockable(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SkinUnlocked(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerSkin(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerExiting(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Teleport(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetViewpoint(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SpawnObject(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SpawnObjectForced(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SpawnProjectile(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_TrackObjectAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_StopTrackingObjectAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Check2DMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Set2DMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerEmeralds(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerLap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_LowestLap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_RingSlingerMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_CaptureTheFlagMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_TeamGame(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ModeAttacking(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_RecordAttack(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_NiGHTSAttack(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_GetObjectX(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectY(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectZ(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectVelX(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectVelY(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectVelZ(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectRoll(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectPitch(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectFloorZ(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectCeilingZ(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectFloorTexture(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectLightLevel(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_CheckObjectState(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_CheckObjectFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectClass(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_SetObjectPosition(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetObjectVelocity(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetObjectAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetObjectRoll(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetObjectPitch(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetObjectState(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetObjectFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetObjectClass(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetObjectSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_MapWarp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_AddBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_RemoveBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_ExitLevel(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_MusicPlay(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_MusicStopAll(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_MusicRestore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_MusicDim(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_GetLineProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetLineProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetSideProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetSideProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetSectorProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetSectorProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GetThingProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_SetThingProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_CheckPowerUp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GivePowerUp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_CheckAmmo(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GiveAmmo(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_TakeAmmo(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_DoSuperTransformation(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_DoSuperDetransformation(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_GiveRings(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GiveSpheres(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GiveLives(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_GiveScore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_DropFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_TossFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_DropEmeralds(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_TossEmeralds(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerHoldingFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerIsIt(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_PlayerFinished(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

bool CallFunc_Sin(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Cos(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Tan(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Arcsin(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Arccos(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Hypot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Sqrt(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Floor(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Ceil(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_Round(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_InvAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);
bool CallFunc_OppositeColor(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC);

#endif // __SRB2_ACS_CALL_FUNCS_HPP__
