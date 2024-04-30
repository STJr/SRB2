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
/// \file  environment.cpp
/// \brief Action Code Script: Environment definition

#include <algorithm>
#include <vector>

#include "acsvm.hpp"

#include "../doomtype.h"
#include "../doomdef.h"
#include "../doomstat.h"

#include "../r_defs.h"
#include "../r_state.h"
#include "../g_game.h"
#include "../p_spec.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../p_local.h"
#include "../p_spec.h"

#include "environment.hpp"
#include "thread.hpp"
#include "call-funcs.hpp"
#include "../cxxutil.hpp"

using namespace srb2::acs;

Environment ACSEnv;

Environment::Environment()
{
	ACSVM::GlobalScope *global = getGlobalScope(0);

	// Activate global scope immediately, since we don't want it off.
	// Not that we're adding any modules to it, though. :p
	global->active = true;

	// Add the data & function pointers.

	// Starting with raw ACS0 codes. I'm using this classic-style
	// format here to have a blueprint for what needs implementing,
	// but it'd also be fine to move these to new style.

	// See also:
	// - https://doomwiki.org/wiki/ACS0_instruction_set
	// - https://github.com/DavidPH/ACSVM/blob/master/ACSVM/CodeData.hpp
	// - https://github.com/DavidPH/ACSVM/blob/master/ACSVM/CodeList.hpp

	//  0 to 56: Implemented by ACSVM
	addCodeDataACS0( 57, {"",        2, addCallFunc(CallFunc_Random)});
	addCodeDataACS0( 58, {"WW",      0, addCallFunc(CallFunc_Random)});
	addCodeDataACS0( 59, {"",        2, addCallFunc(CallFunc_ThingCount)});
	addCodeDataACS0( 60, {"WW",      0, addCallFunc(CallFunc_ThingCount)});
	addCodeDataACS0( 61, {"",        1, addCallFunc(CallFunc_TagWait)});
	addCodeDataACS0( 62, {"W",       0, addCallFunc(CallFunc_TagWait)});
	addCodeDataACS0( 63, {"",        1, addCallFunc(CallFunc_PolyWait)});
	addCodeDataACS0( 64, {"W",       0, addCallFunc(CallFunc_PolyWait)});
	addCodeDataACS0( 65, {"",        2, addCallFunc(CallFunc_ChangeFloor)});
	addCodeDataACS0( 66, {"WWS",     0, addCallFunc(CallFunc_ChangeFloor)});
	addCodeDataACS0( 67, {"",        2, addCallFunc(CallFunc_ChangeCeiling)});
	addCodeDataACS0( 68, {"WWS",     0, addCallFunc(CallFunc_ChangeCeiling)});
	// 69 to 79: Implemented by ACSVM
	addCodeDataACS0( 80, {"",        0, addCallFunc(CallFunc_LineSide)});
	// 81 to 82: Implemented by ACSVM
	addCodeDataACS0( 83, {"",        0, addCallFunc(CallFunc_ClearLineSpecial)});
	// 84 to 85: Implemented by ACSVM
	addCodeDataACS0( 86, {"",        0, addCallFunc(CallFunc_EndPrint)});
	// 87 to 89: Implemented by ACSVM
	addCodeDataACS0( 90, {"",        0, addCallFunc(CallFunc_PlayerCount)});
	addCodeDataACS0( 91, {"",        0, addCallFunc(CallFunc_GameType)});
	// addCodeDataACS0( 92, {"",        0, addCallFunc(CallFunc_GameSkill)});
	addCodeDataACS0( 93, {"",        0, addCallFunc(CallFunc_Timer)});
	addCodeDataACS0( 94, {"",        2, addCallFunc(CallFunc_SectorSound)});
	addCodeDataACS0( 95, {"",        2, addCallFunc(CallFunc_AmbientSound)});

	addCodeDataACS0( 97, {"",        4, addCallFunc(CallFunc_SetLineTexture)});

	addCodeDataACS0( 99, {"",        7, addCallFunc(CallFunc_SetLineSpecial)});
	addCodeDataACS0(100, {"",        3, addCallFunc(CallFunc_ThingSound)});
	addCodeDataACS0(101, {"",        0, addCallFunc(CallFunc_EndPrintBold)});
	// Hexen p-codes end here

	// Skulltag p-codes begin here
	addCodeDataACS0(118, {"",        0, addCallFunc(CallFunc_IsNetworkGame)});
	addCodeDataACS0(119, {"",        0, addCallFunc(CallFunc_PlayerTeam)});

	// 136 to 137: Implemented by ACSVM
	addCodeDataACS0(149, {"",        6, addCallFunc(CallFunc_SpawnObject)});

	// 157: Implemented by ACSVM

	// 167 to 173: Implemented by ACSVM
	addCodeDataACS0(174, {"BB",      0, addCallFunc(CallFunc_Random)});
	// 175 to 179: Implemented by ACSVM

	addCodeDataACS0(180, {"",        7, addCallFunc(CallFunc_SetObjectSpecial)});

	// 181 to 189: Implemented by ACSVM
	addCodeDataACS0(196, {"",        1, addCallFunc(CallFunc_GetObjectX)});
	addCodeDataACS0(197, {"",        1, addCallFunc(CallFunc_GetObjectY)});
	addCodeDataACS0(198, {"",        1, addCallFunc(CallFunc_GetObjectZ)});

	// 203 to 217: Implemented by ACSVM
	addCodeDataACS0(220, {"",        2, addCallFunc(CallFunc_Sin)});
	addCodeDataACS0(221, {"",        2, addCallFunc(CallFunc_Cos)});

	// 225 to 243: Implemented by ACSVM
	addCodeDataACS0(245, {"",        3, addCallFunc(CallFunc_SetThingProperty)});
	addCodeDataACS0(246, {"",        2, addCallFunc(CallFunc_GetThingProperty)});
	addCodeDataACS0(247, {"",        0, addCallFunc(CallFunc_PlayerNumber)});
	addCodeDataACS0(248, {"",        0, addCallFunc(CallFunc_ActivatorTID)});

	// 253: Implemented by ACSVM

	// 256 to 257: Implemented by ACSVM
	addCodeDataACS0(259, {"",        1, addCallFunc(CallFunc_GetObjectFloorZ)});

	// 263: Implemented by ACSVM
	addCodeDataACS0(260, {"",        1, addCallFunc(CallFunc_GetObjectAngle)});
	addCodeDataACS0(266, {"",        2, addCallFunc(CallFunc_ChangeSky)}); // reimplements linedef type 423
	addCodeDataACS0(270, {"",        0, addCallFunc(CallFunc_EndLog)});
	// 273 to 275: Implemented by ACSVM
	addCodeDataACS0(276, {"",        2, addCallFunc(CallFunc_SetObjectAngle)});
	addCodeDataACS0(280, {"",        7, addCallFunc(CallFunc_SpawnProjectile)});
	addCodeDataACS0(282, {"",        1, addCallFunc(CallFunc_GetObjectCeilingZ)});

	// 291 to 325: Implemented by ACSVM

	// 330: Implemented by ACSVM
	addCodeDataACS0(331, {"",        1, addCallFunc(CallFunc_GetObjectPitch)});
	addCodeDataACS0(332, {"",        1, addCallFunc(CallFunc_SetObjectPitch)});
	addCodeDataACS0(334, {"",        1, addCallFunc(CallFunc_SetObjectState)});
	addCodeDataACS0(340, {"",        1, addCallFunc(CallFunc_GetObjectLightLevel)});

	// 349 to 361: Implemented by ACSVM

	// 363 to 381: Implemented by ACSVM

	// Now for new style functions.
	// This style is preferred for added functions
	// that aren't mimicing one from Hexen's or ZDoom's
	// ACS implementations.
	addFuncDataACS0(   1, addCallFunc(CallFunc_GetLineProperty));
	addFuncDataACS0(   2, addCallFunc(CallFunc_SetLineProperty));
	// addFuncDataACS0(   3, addCallFunc(CallFunc_GetLineUserProperty));
	addFuncDataACS0(   4, addCallFunc(CallFunc_GetSectorProperty));
	addFuncDataACS0(   5, addCallFunc(CallFunc_SetSectorProperty));
	// addFuncDataACS0(   6, addCallFunc(CallFunc_GetSectorUserProperty));
	addFuncDataACS0(   7, addCallFunc(CallFunc_GetSideProperty));
	addFuncDataACS0(   8, addCallFunc(CallFunc_SetSideProperty));
	// addFuncDataACS0(   9, addCallFunc(CallFunc_GetSideUserProperty));
	// addFuncDataACS0(  12, addCallFunc(CallFunc_GetThingUserProperty));
	// addFuncDataACS0(  13, addCallFunc(CallFunc_GetPlayerProperty));
	// addFuncDataACS0(  14, addCallFunc(CallFunc_SetPlayerProperty));
	// addFuncDataACS0(  15, addCallFunc(CallFunc_GetPolyobjProperty));
	// addFuncDataACS0(  16, addCallFunc(CallFunc_SetPolyobjProperty));

	addFuncDataACS0( 100, addCallFunc(CallFunc_strcmp));
	addFuncDataACS0( 101, addCallFunc(CallFunc_strcasecmp));

	addFuncDataACS0( 120, addCallFunc(CallFunc_PlayerRings));
	addFuncDataACS0( 122, addCallFunc(CallFunc_PlayerScore));
	addFuncDataACS0( 123, addCallFunc(CallFunc_PlayerSuper));

	addFuncDataACS0( 200, addCallFunc(CallFunc_GetObjectVelX));
	addFuncDataACS0( 201, addCallFunc(CallFunc_GetObjectVelY));
	addFuncDataACS0( 202, addCallFunc(CallFunc_GetObjectVelZ));
	addFuncDataACS0( 203, addCallFunc(CallFunc_GetObjectRoll));
	addFuncDataACS0( 204, addCallFunc(CallFunc_GetObjectFloorTexture));
	addFuncDataACS0( 205, addCallFunc(CallFunc_CheckObjectState));
	addFuncDataACS0( 206, addCallFunc(CallFunc_CheckObjectFlag));
	addFuncDataACS0( 207, addCallFunc(CallFunc_GetObjectClass));
	addFuncDataACS0( 208, addCallFunc(CallFunc_GetObjectDye));

	addFuncDataACS0( 209, addCallFunc(CallFunc_SetObjectVelocity));
	addFuncDataACS0( 210, addCallFunc(CallFunc_SetObjectRoll));
	addFuncDataACS0( 211, addCallFunc(CallFunc_SetObjectFlag));
	addFuncDataACS0( 212, addCallFunc(CallFunc_SetObjectClass));
	addFuncDataACS0( 213, addCallFunc(CallFunc_SetObjectDye));
	addFuncDataACS0( 214, addCallFunc(CallFunc_SpawnObjectForced));

	addFuncDataACS0( 300, addCallFunc(CallFunc_CountEnemies));
	addFuncDataACS0( 301, addCallFunc(CallFunc_CountPushables));
	addFuncDataACS0( 302, addCallFunc(CallFunc_HasUnlockable));
	addFuncDataACS0( 303, addCallFunc(CallFunc_SkinUnlocked));
	addFuncDataACS0( 304, addCallFunc(CallFunc_PlayerSkin));
	addFuncDataACS0( 305, addCallFunc(CallFunc_PlayerEmeralds));
	addFuncDataACS0( 306, addCallFunc(CallFunc_PlayerBot));
	addFuncDataACS0( 307, addCallFunc(CallFunc_PlayerExiting));
	addFuncDataACS0( 308, addCallFunc(CallFunc_PlayerLap));
	addFuncDataACS0( 309, addCallFunc(CallFunc_RingSlingerMode));
	addFuncDataACS0( 310, addCallFunc(CallFunc_TeamGame));
	addFuncDataACS0( 311, addCallFunc(CallFunc_CaptureTheFlagMode));
	addFuncDataACS0( 312, addCallFunc(CallFunc_RecordAttack));
	addFuncDataACS0( 313, addCallFunc(CallFunc_NiGHTSAttack));
	addFuncDataACS0( 314, addCallFunc(CallFunc_ModeAttacking));
	addFuncDataACS0( 315, addCallFunc(CallFunc_LowestLap));
	addFuncDataACS0( 322, addCallFunc(CallFunc_Teleport));
	addFuncDataACS0( 323, addCallFunc(CallFunc_SetViewpoint));
	addFuncDataACS0( 325, addCallFunc(CallFunc_TrackObjectAngle));
	addFuncDataACS0( 326, addCallFunc(CallFunc_StopTrackingObjectAngle));
	addFuncDataACS0( 327, addCallFunc(CallFunc_CheckPowerUp));
	addFuncDataACS0( 328, addCallFunc(CallFunc_GivePowerUp));
	addFuncDataACS0( 329, addCallFunc(CallFunc_CheckAmmo));
	addFuncDataACS0( 330, addCallFunc(CallFunc_GiveAmmo));
	addFuncDataACS0( 331, addCallFunc(CallFunc_TakeAmmo));
	addFuncDataACS0( 332, addCallFunc(CallFunc_DoSuperTransformation));
	addFuncDataACS0( 333, addCallFunc(CallFunc_DoSuperDetransformation));
	addFuncDataACS0( 334, addCallFunc(CallFunc_GiveRings));
	addFuncDataACS0( 335, addCallFunc(CallFunc_GiveSpheres));
	addFuncDataACS0( 336, addCallFunc(CallFunc_GiveLives));
	addFuncDataACS0( 337, addCallFunc(CallFunc_GiveScore));
	addFuncDataACS0( 338, addCallFunc(CallFunc_DropFlag));
	addFuncDataACS0( 339, addCallFunc(CallFunc_TossFlag));
	addFuncDataACS0( 340, addCallFunc(CallFunc_DropEmeralds));
	addFuncDataACS0( 341, addCallFunc(CallFunc_TossEmeralds));
	addFuncDataACS0( 342, addCallFunc(CallFunc_PlayerHoldingFlag));
	addFuncDataACS0( 343, addCallFunc(CallFunc_PlayerIsIt));
	addFuncDataACS0( 344, addCallFunc(CallFunc_PlayerFinished));

	addFuncDataACS0( 500, addCallFunc(CallFunc_CameraWait));
	addFuncDataACS0( 503, addCallFunc(CallFunc_SetLineRenderStyle));
	addFuncDataACS0( 504, addCallFunc(CallFunc_MapWarp));
	addFuncDataACS0( 505, addCallFunc(CallFunc_AddBot));
	addFuncDataACS0( 506, addCallFunc(CallFunc_RemoveBot));
	addFuncDataACS0( 507, addCallFunc(CallFunc_ExitLevel));
	addFuncDataACS0( 508, addCallFunc(CallFunc_MusicPlay));
	addFuncDataACS0( 509, addCallFunc(CallFunc_MusicStopAll));
	addFuncDataACS0( 510, addCallFunc(CallFunc_MusicRestore));
	addFuncDataACS0( 512, addCallFunc(CallFunc_MusicDim));

	// addFuncDataACS0( 600, addCallFunc(CallFunc_StartConversation));

	addFuncDataACS0( 700, addCallFunc(CallFunc_Tan));
	addFuncDataACS0( 701, addCallFunc(CallFunc_Arcsin));
	addFuncDataACS0( 702, addCallFunc(CallFunc_Arccos));
	addFuncDataACS0( 703, addCallFunc(CallFunc_Hypot));
	addFuncDataACS0( 704, addCallFunc(CallFunc_Sqrt));
	addFuncDataACS0( 705, addCallFunc(CallFunc_Floor));
	addFuncDataACS0( 706, addCallFunc(CallFunc_Ceil));
	addFuncDataACS0( 707, addCallFunc(CallFunc_Round));
	addFuncDataACS0( 710, addCallFunc(CallFunc_InvAngle));
	addFuncDataACS0( 711, addCallFunc(CallFunc_OppositeColor));
}

ACSVM::Thread *Environment::allocThread()
{
	return new Thread(this);
}

ACSVM::ModuleName Environment::getModuleName(char const *str, size_t len)
{
	ACSVM::String *name = getString(str, len);
	lumpnum_t lump = W_CheckNumForNameInFolder(str, "ACS/");

	return { name, nullptr, static_cast<size_t>(lump) };
}

void Environment::loadModule(ACSVM::Module *module)
{
	ACSVM::ModuleName *const name = &module->name;

	size_t lumpLen = 0;
	std::vector<ACSVM::Byte> data;

	if (name->i == (size_t)LUMPERROR)
	{
		// No lump given for module.
		CONS_Alert(CONS_WARNING, "Could not find ACS module \"%s\"; scripts will not function properly!\n", name->s->str);
		return; //throw ACSVM::ReadError("file open failure");
	}

	lumpLen = W_LumpLength(name->i);

	if (W_IsLumpWad(name->i) == true || lumpLen == 0)
	{
		CONS_Debug(DBG_SETUP, "Attempting to load ACS module from the BEHAVIOR lump of map '%s'...\n", name->s->str);

		// The lump given is a virtual resource.
		// Try to grab a BEHAVIOR lump from inside of it.
		virtres_t *vRes = vres_GetMap(name->i);
		auto _ = srb2::finally([vRes]() { vres_Free(vRes); });

		virtlump_t *vLump = vres_Find(vRes, "BEHAVIOR");
		if (vLump != nullptr && vLump->size > 0)
		{
			data.insert(data.begin(), vLump->data, vLump->data + vLump->size);
			CONS_Debug(DBG_SETUP, "Successfully found BEHAVIOR lump.\n");
		}
		else
		{
			CONS_Debug(DBG_SETUP, "No BEHAVIOR lump found.\n");
		}
	}
	else
	{
		CONS_Debug(DBG_SETUP, "Loading ACS module directly from lump '%s'...\n", name->s->str);

		// It's a real lump.
		ACSVM::Byte *lump = static_cast<ACSVM::Byte *>(Z_Calloc(lumpLen, PU_STATIC, nullptr));
		auto _ = srb2::finally([lump]() { Z_Free(lump); });

		W_ReadLump(name->i, lump);
		data.insert(data.begin(), lump, lump + lumpLen);
	}

	if (data.empty() == false)
	{
		try
		{
			module->readBytecode(data.data(), data.size());
		}
		catch (const ACSVM::ReadError &e)
		{
			CONS_Alert(CONS_ERROR, "Failed to load ACS module '%s': %s\n", name->s->str, e.what());
			throw ACSVM::ReadError("failed import");
		}
	}
	else
	{
		// Unlike Hexen, a BEHAVIOR lump is not required.
		// Simply ignore in this instance.
		CONS_Debug(DBG_SETUP, "ACS module has no data, ignoring...\n");
	}
}

bool Environment::checkTag(ACSVM::Word type, ACSVM::Word tag)
{
	switch (type)
	{
		case ACS_TAGTYPE_SECTOR:
		{
			INT32 secnum = -1;

			TAG_ITER_SECTORS(tag, secnum)
			{
				sector_t *sec = &sectors[secnum];

				if (sec->floordata != nullptr || sec->ceilingdata != nullptr)
				{
					return false;
				}
			}

			return true;
		}

		case ACS_TAGTYPE_POLYOBJ:
		{
			const polyobj_t *po = Polyobj_GetForNum(tag);
			return (po == nullptr || po->thinker == nullptr);
		}

		case ACS_TAGTYPE_CAMERA:
		{
			const mobj_t *camera = P_FindObjectTypeFromTag(MT_ALTVIEWMAN, tag);
			if (camera == nullptr)
			{
				return true;
			}

			return (camera->tracer == nullptr || P_MobjWasRemoved(camera->tracer) == true);
		}
	}

	return true;
}

ACSVM::Word Environment::callSpecImpl
	(
		ACSVM::Thread *thread, ACSVM::Word spec,
		const ACSVM::Word *argV, ACSVM::Word argC
	)
{
	auto info = &static_cast<Thread *>(thread)->info;
	ACSVM::MapScope *const map = thread->scopeMap;

	INT32 args[NUM_SCRIPT_ARGS] = {0};

	char *stringargs[NUM_SCRIPT_STRINGARGS] = {0};
	auto _ = srb2::finally(
		[stringargs]()
		{
			for (int i = 0; i < NUM_SCRIPT_STRINGARGS; i++)
			{
				Z_Free(stringargs[i]);
			}
		}
	);

	activator_t *activator = static_cast<activator_t *>(Z_Calloc(sizeof(activator_t), PU_LEVEL, nullptr));
	auto __ = srb2::finally(
		[info, activator]()
		{
			if (info->thread_era == thinker_era)
			{
				P_SetTarget(&activator->mo, NULL);
				Z_Free(activator);
			}
		}
	);

	int i = 0;

	for (i = 0; i < std::min((signed)(argC), NUM_SCRIPT_STRINGARGS); i++)
	{
		ACSVM::String *strPtr = map->getString(argV[i]);

		stringargs[i] = static_cast<char *>(Z_Malloc(strPtr->len + 1, PU_STATIC, nullptr));
		M_Memcpy(stringargs[i], strPtr->str, strPtr->len + 1);
	}

	for (i = 0; i < std::min((signed)(argC), NUM_SCRIPT_ARGS); i++)
	{
		args[i] = argV[i];
	}

	P_SetTarget(&activator->mo, info->mo);
	activator->line = info->line;
	activator->side = info->side;
	activator->sector = info->sector;
	activator->po = info->po;
	activator->fromLineSpecial = false;

	P_ProcessSpecial(activator, spec, args, stringargs);
	return 1;
}

void Environment::printKill(ACSVM::Thread *thread, ACSVM::Word type, ACSVM::Word data)
{
	CONS_Alert(CONS_ERROR, "ACSVM ERROR: Kill %u:%d at %lu\n", type, data, (thread->codePtr - thread->module->codeV.data() - 1));
}
