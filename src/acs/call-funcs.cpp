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
/// \file  call-funcs.cpp
/// \brief Action Code Script: CallFunc instructions

#include <algorithm>
#include <cctype>

#include "acsvm.hpp"

#include "../doomtype.h"
#include "../doomdef.h"
#include "../doomstat.h"

#include "../d_think.h"
#include "../p_mobj.h"
#include "../p_tick.h"
#include "../f_finale.h"
#include "../w_wad.h"
#include "../m_misc.h"
#include "../m_random.h"
#include "../g_game.h"
#include "../d_player.h"
#include "../r_defs.h"
#include "../r_state.h"
#include "../r_main.h"
#include "../p_polyobj.h"
#include "../taglist.h"
#include "../p_local.h"
#include "../b_bot.h"
#include "../info.h"
#include "../deh_tables.h"
#include "../fastcmp.h"
#include "../hu_stuff.h"
#include "../s_sound.h"
#include "../r_textures.h"
#include "../m_fixed.h"
#include "../m_cond.h"
#include "../r_skins.h"
#include "../z_zone.h"
#include "../s_sound.h"
#include "../r_draw.h"
#include "../r_fps.h"
#include "../netcode/net_command.h"

#include "call-funcs.hpp"

#include "environment.hpp"
#include "thread.hpp"
#include "../cxxutil.hpp"

using namespace srb2::acs;

#define NO_RETURN(thread) thread->dataStk.push(0)

/*--------------------------------------------------
	static bool ACS_GetMobjTypeFromString(const char *word, mobjtype_t *type)

		Helper function for CallFunc_ThingCount. Gets
		an object type from a string.

	Input Arguments:-
		word: The mobj class string.
		type: Variable to store the result in.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static bool ACS_GetMobjTypeFromString(const char *word, mobjtype_t *type)
{
	if (fastncmp("MT_", word, 3))
	{
		// take off the MT_
		word += 3;
	}

	for (int i = 0; i < NUMMOBJFREESLOTS; i++)
	{
		if (!FREE_MOBJS[i])
		{
			break;
		}

		if (fastcmp(word, FREE_MOBJS[i]))
		{
			*type = static_cast<mobjtype_t>(static_cast<int>(MT_FIRSTFREESLOT) + i);
			return true;
		}
	}

	for (int i = 0; i < MT_FIRSTFREESLOT; i++)
	{
		if (fastcmp(word, MOBJTYPE_LIST[i] + 3))
		{
			*type = static_cast<mobjtype_t>(i);
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	static bool ACS_GetSFXFromString(const char *word, sfxenum_t *type)

		Helper function for sound playing functions.
		Gets a SFX id from a string.

	Input Arguments:-
		word: The sound effect string.
		type: Variable to store the result in.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static bool ACS_GetSFXFromString(const char *word, sfxenum_t *type)
{
	if (fastnicmp("SFX_", word, 4)) // made case insensitive
	{
		// take off the SFX_
		word += 4;
	}
	else if (fastnicmp("DS", word, 2)) // made case insensitive
	{
		// take off the DS
		word += 2;
	}

	for (int i = 0; i < NUMSFX; i++)
	{
		if (S_sfx[i].name && fasticmp(word, S_sfx[i].name))
		{
			*type = static_cast<sfxenum_t>(i);
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	static bool ACS_GetSpriteFromString(const char *word, spritenum_t *type)

		Helper function for CallFunc_Get/SetThingProperty.
		Gets a sprite from a string.

	Input Arguments:-
		word: The sprite string.
		type: Variable to store the result in.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static bool ACS_GetSpriteFromString(const char *word, spritenum_t *type)
{
	if (fastncmp("SPR_", word, 4))
	{
		// take off the SPR_
		word += 4;
	}

	for (int i = 0; i < NUMSPRITES; i++)
	{
		if (strcmp(word, sprnames[i]) == 0)
		{
			*type = static_cast<spritenum_t>(i);
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	static bool ACS_GetSprite2FromString(const char *word, playersprite_t *type)

		Helper function for CallFunc_Get/SetThingProperty.
		Gets a sprite2 from a string.

	Input Arguments:-
		word: The sprite2 string.
		type: Variable to store the result in.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static bool ACS_GetSprite2FromString(const char *word, playersprite_t *type)
{
	if (fastncmp("SPR2_", word, 5))
	{
		// take off the SPR2_
		word += 5;
	}

	for (int i = 0; i < free_spr2; i++)
	{
		if (fastcmp(word, spr2names[i]))
		{
			*type = static_cast<playersprite_t>(i);
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	static bool ACS_GetStateFromString(const char *word, playersprite_t *type)

		Helper function for CallFunc_Get/SetThingProperty.
		Gets a state from a string.

	Input Arguments:-
		word: The state string.
		type: Variable to store the result in.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static bool ACS_GetStateFromString(const char *word, statenum_t *type)
{
	if (fastncmp("S_", word, 2))
	{
		// take off the S_
		word += 2;
	}

	for (int i = 0; i < NUMMOBJFREESLOTS; i++)
	{
		if (!FREE_STATES[i])
		{
			break;
		}

		if (fastcmp(word, FREE_STATES[i]))
		{
			*type = static_cast<statenum_t>(static_cast<int>(S_FIRSTFREESLOT) + i);
			return true;
		}
	}

	for (int i = 0; i < S_FIRSTFREESLOT; i++)
	{
		if (fastcmp(word, STATE_LIST[i] + 2))
		{
			*type = static_cast<statenum_t>(i);
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	static bool ACS_GetSkinFromString(const char *word, INT32 *type)

		Helper function for CallFunc_Get/SetThingProperty.
		Gets a skin from a string.

	Input Arguments:-
		word: The skin string.
		type: Variable to store the result in.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static bool ACS_GetSkinFromString(const char *word, INT32 *type)
{
	INT32 skin = R_SkinAvailable(word);
	if (skin == -1)
		return false;

	*type = skin;
	return true;
}

/*--------------------------------------------------
	static bool ACS_GetColorFromString(const char *word, skincolornum_t *type)

		Helper function for CallFunc_Get/SetThingProperty.
		Gets a color from a string.

	Input Arguments:-
		word: The color string.
		type: Variable to store the result in.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static bool ACS_GetColorFromString(const char *word, skincolornum_t *type)
{
	for (int i = 0; i < numskincolors; i++)
	{
		if (fastcmp(word, skincolors[i].name))
		{
			*type = static_cast<skincolornum_t>(i);
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	static bool ACS_CheckActorFlag(mobj_t *mobj, mobjtype_t type)

		Helper function for CallFunc_SetObjectFlag and CallFunc_CheckObjectFlag.

	Input Arguments:-
		flag: The name of the flag to check.
		result: Which flag to give to the actor.
		type: Variable to store which flag set the actor flag was found it.

	Return:-
		true if the flag exists, otherwise false.
--------------------------------------------------*/
static bool ACS_CheckActorFlag(const char *flag, UINT32 *result, unsigned *type)
{
	if (result)
		*result = 0;
	if (type)
		*type = 0;

	// Could be optimized but I don't care right now. First check regular flags
	for (unsigned i = 0; MOBJFLAG_LIST[i]; i++)
	{
		if (fastcmp(flag, MOBJFLAG_LIST[i]))
		{
			if (result)
				*result = (1 << i);
			if (type)
				*type = 1;
			return true;
		}
	}

	// Now check flags2
	for (unsigned i = 0; MOBJFLAG2_LIST[i]; i++)
	{
		if (fastcmp(flag, MOBJFLAG2_LIST[i]))
		{
			if (result)
				*result = (1 << i);
			if (type)
				*type = 2;
			return true;
		}
	}

	// Finally, check extra flags
	for (unsigned i = 0; MOBJEFLAG_LIST[i]; i++)
	{
		if (fastcmp(flag, MOBJEFLAG_LIST[i]))
		{
			if (result)
				*result = (1 << i);
			if (type)
				*type = 3;
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	static angle_t ACS_FixedToAngle(int angle)

		Converts a fixed-point angle to a Doom angle.
--------------------------------------------------*/
static angle_t ACS_FixedToAngle(fixed_t angle)
{
	return FixedAngle(angle * 360);
}

/*--------------------------------------------------
	static void ACS_AngleToFixed(ACSVM::Thread *thread, angle_t angle)

		Converts a Doom angle to a fixed-point angle.
--------------------------------------------------*/
static fixed_t ACS_AngleToFixed(angle_t angle)
{
	return FixedDiv(AngleFixed(angle), 360*FRACUNIT);
}

/*--------------------------------------------------
	static bool ACS_CountThing(mobj_t *mobj, mobjtype_t type)

		Helper function for CallFunc_ThingCount.
		Returns whenever or not to add this thing
		to the thing count.

	Input Arguments:-
		mobj: The mobj we want to count.
		type: Type exclusion.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static bool ACS_CountThing(mobj_t *mobj, mobjtype_t type)
{
	if (type == MT_NULL || mobj->type == type)
	{
		// Don't count dead monsters
		if (mobj->info->spawnhealth > 0 && mobj->health <= 0)
		{
			// Note: Hexen checks for COUNTKILL.
			// SRB2 does not have an equivalent, so I'm checking
			// spawnhealth. Feel free to replace this condition
			// with literally anything else.
			return false;
		}

		// Count this object.
		return true;
	}

	return false;
}

/*--------------------------------------------------
	static bool ACS_ActivatorIsLocal(ACSVM::Thread *thread)

		Helper function for many print functions.
		Returns whenever or not the activator of the
		thread is a display player or not.

	Input Arguments:-
		thread: The thread we're exeucting on.

	Return:-
		true if it's for a display player,
		otherwise false.
--------------------------------------------------*/
static bool ACS_ActivatorIsLocal(ACSVM::Thread *thread)
{
	auto info = &static_cast<Thread *>(thread)->info;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		return info->mo->player == &players[displayplayer];
	}

	return false;
}

/*--------------------------------------------------
	static UINT32 ACS_SectorThingCounter(sector_t *sec, mtag_t thingTag, bool (*filter)(mobj_t *))

		Helper function for CallFunc_CountEnemies
		and CallFunc_CountPushables. Counts a number
		of things in the specified sector.

	Input Arguments:-
		sec: The sector to search in.
		thingTag: Thing tag to filter for. 0 allows any.
		filter: Filter function, total count is increased when
			this function returns true.

	Return:-
		Numbers of things matching the filter found.
--------------------------------------------------*/
static UINT32 ACS_SectorThingCounter(sector_t *sec, mtag_t thingTag, bool (*filter)(mobj_t *))
{
	UINT32 count = 0;

	for (msecnode_t *node = sec->touching_thinglist; node; node = node->m_thinglist_next) // things touching this sector
	{
		mobj_t *mo = node->m_thing;

		if (thingTag != 0 && mo->tid != thingTag)
		{
			continue;
		}

		if (mo->z > sec->ceilingheight
			|| mo->z + mo->height < sec->floorheight)
		{
			continue;
		}

		if (filter(mo) == true)
		{
			count++;
		}
	}

	return count;
}

/*--------------------------------------------------
	static UINT32 ACS_SectorTagThingCounter(mtag_t sectorTag, sector_t *activator, mtag_t thingTag, bool (*filter)(mobj_t *))

		Helper function for CallFunc_CountEnemies
		and CallFunc_CountPushables. Counts a number
		of things in the tagged sectors.

	Input Arguments:-
		sectorTag: The sector tag to search in.
		activator: The activator sector to fall back on when sectorTag is 0.
		thingTag: Thing tag to filter for. 0 allows any.
		filter: Filter function, total count is increased when
			this function returns true.

	Return:-
		Numbers of things matching the filter found.
--------------------------------------------------*/
static UINT32 ACS_SectorIterateThingCounter(sector_t *sec, mtag_t thingTag, bool (*filter)(mobj_t *))
{
	UINT32 count = 0;
	boolean FOFsector = false;
	size_t i;

	if (sec == nullptr)
	{
		return 0;
	}

	// Check the lines of this sector, to see if it is a FOF control sector.
	for (i = 0; i < sec->linecount; i++)
	{
		INT32 targetsecnum = -1;

		if (sec->lines[i]->special < 100 || sec->lines[i]->special >= 300)
		{
			continue;
		}

		FOFsector = true;

		TAG_ITER_SECTORS(sec->lines[i]->args[0], targetsecnum)
		{
			sector_t *targetsec = &sectors[targetsecnum];
			count += ACS_SectorThingCounter(targetsec, thingTag, filter);
		}
	}

	if (FOFsector == false)
	{
		count += ACS_SectorThingCounter(sec, thingTag, filter);
	}

	return count;
}

static UINT32 ACS_SectorTagThingCounter(mtag_t sectorTag, sector_t *activator, mtag_t thingTag, bool (*filter)(mobj_t *))
{
	UINT32 count = 0;

	if (sectorTag == 0)
	{
		count += ACS_SectorIterateThingCounter(activator, thingTag, filter);
	}
	else
	{
		INT32 secnum = -1;

		TAG_ITER_SECTORS(sectorTag, secnum)
		{
			sector_t *sec = &sectors[secnum];
			count += ACS_SectorIterateThingCounter(sec, thingTag, filter);
		}
	}

	return count;
}

/*--------------------------------------------------
	static bool ACS_IsLocationValid(mobj_t *mobj, fixed_t x, fixed_t y, fixed_t z)

		Helper function for CallFunc_SpawnObject and CallFunc_SetObjectPosition.
		Checks if the given coordinates are valid for an actor to exist in.
--------------------------------------------------*/
static bool ACS_IsLocationValid(mobj_t *mobj, fixed_t x, fixed_t y, fixed_t z)
{
	if (P_CheckPosition(mobj, x, y) == true)
	{
		fixed_t floorz = P_FloorzAtPos(x, y, z, mobj->height);
		fixed_t ceilingz = P_CeilingzAtPos(x, y, z, mobj->height);
		if (z >= floorz && (z + mobj->height) <= ceilingz)
		{
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	bool CallFunc_Random(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		ACS wrapper for P_RandomRange.
--------------------------------------------------*/
bool CallFunc_Random(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	INT32 low = 0;
	INT32 high = 0;

	(void)argC;

	low = argV[0];
	high = argV[1];

	thread->dataStk.push(P_RandomRange(low, high));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_ThingCount(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Counts the number of things of a particular
		type and tid. Both fields are optional;
		no type means indescriminate against type,
		no tid means search thru all thinkers.
--------------------------------------------------*/
static mobjtype_t filter_for_mobjtype = MT_NULL; // annoying but I don't wanna mess with other code
static bool ACS_ThingTypeFilter(mobj_t *mo)
{
	return (ACS_CountThing(mo, filter_for_mobjtype));
}

bool CallFunc_ThingCount(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = NULL;
	ACSVM::String *str = NULL;
	const char *className = NULL;
	size_t classLen = 0;

	mobjtype_t type = MT_NULL;
	mtag_t tid = 0;
	mtag_t sectorTag = 0;

	size_t count = 0;

	map = thread->scopeMap;
	str = map->getString(argV[0]);

	className = str->str;
	classLen = str->len;

	if (classLen > 0)
	{
		bool success = ACS_GetMobjTypeFromString(className, &type);

		if (success == false)
		{
			// Exit early.
			CONS_Alert(CONS_WARNING,
				"Couldn't find actor class \"%s\" for ThingCount.\n",
				className
			);

			NO_RETURN(thread);

			return false;
		}
	}
	else
	{
		CONS_Alert(CONS_WARNING, "ThingCount actor class was not provided.\n");

		NO_RETURN(thread);

		return false;
	}

	tid = argV[1];

	if (argC > 2)
	{
		sectorTag = argV[2];
	}

	if (sectorTag != 0)
	{
		// Search through sectors.
		filter_for_mobjtype = type;
		count = ACS_SectorTagThingCounter(sectorTag, nullptr, tid, ACS_ThingTypeFilter);
		filter_for_mobjtype = MT_NULL;
	}
	else if (tid != 0)
	{
		// Search through tag lists.
		mobj_t *mobj = nullptr;

		while ((mobj = P_FindMobjFromTID(tid, mobj, nullptr)) != nullptr)
		{
			if (ACS_CountThing(mobj, type) == true)
			{
				++count;
			}
		}
	}
	else
	{
		// Search thinkers instead of tag lists.
		thinker_t *th = nullptr;
		mobj_t *mobj = nullptr;

		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			{
				continue;
			}

			mobj = (mobj_t *)th;

			if (ACS_CountThing(mobj, type) == true)
			{
				++count;
			}
		}
	}

	thread->dataStk.push(count);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_TagWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pauses the thread until the tagged
		sector stops moving.
--------------------------------------------------*/
bool CallFunc_TagWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	thread->state = {
		ACSVM::ThreadState::WaitTag,
		argV[0],
		ACS_TAGTYPE_SECTOR
	};

	NO_RETURN(thread);

	return true; // Execution interrupted
}

/*--------------------------------------------------
	bool CallFunc_PolyWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pauses the thread until the tagged
		polyobject stops moving.
--------------------------------------------------*/
bool CallFunc_PolyWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	thread->state = {
		ACSVM::ThreadState::WaitTag,
		argV[0],
		ACS_TAGTYPE_POLYOBJ
	};

	NO_RETURN(thread);

	return true; // Execution interrupted
}

/*--------------------------------------------------
	bool CallFunc_CameraWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pauses the thread until the tagged
		camera is done moving.
--------------------------------------------------*/
bool CallFunc_CameraWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	thread->state = {
		ACSVM::ThreadState::WaitTag,
		argV[0],
		ACS_TAGTYPE_CAMERA
	};

	NO_RETURN(thread);

	return true; // Execution interrupted
}

/*--------------------------------------------------
	bool CallFunc_ChangeFloor(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Changes a floor texture.
--------------------------------------------------*/
bool CallFunc_ChangeFloor(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = nullptr;
	ACSVM::String *str = nullptr;
	const char *texName = nullptr;

	INT32 secnum = -1;
	mtag_t tag = 0;

	(void)argC;

	tag = argV[0];

	map = thread->scopeMap;
	str = map->getString(argV[1]);
	texName = str->str;

	TAG_ITER_SECTORS(tag, secnum)
	{
		sector_t *sec = &sectors[secnum];
		sec->floorpic = P_AddLevelFlatRuntime(texName);
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_ChangeCeiling(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Changes a ceiling texture.
--------------------------------------------------*/
bool CallFunc_ChangeCeiling(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = NULL;
	ACSVM::String *str = NULL;
	const char *texName = NULL;

	INT32 secnum = -1;
	mtag_t tag = 0;

	(void)argC;

	tag = argV[0];

	map = thread->scopeMap;
	str = map->getString(argV[1]);
	texName = str->str;

	TAG_ITER_SECTORS(tag, secnum)
	{
		sector_t *sec = &sectors[secnum];
		sec->ceilingpic = P_AddLevelFlatRuntime(texName);
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_LineSide(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pushes which side of the linedef was
		activated.
--------------------------------------------------*/
bool CallFunc_LineSide(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argV;
	(void)argC;

	thread->dataStk.push(info->side);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_ClearLineSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		If there is an activating linedef, set its
		special to 0.
--------------------------------------------------*/
bool CallFunc_ClearLineSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argV;
	(void)argC;

	if (info->line != NULL)
	{
		// One time only.
		info->line->special = 0;
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_EndPrint(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		One of the ACS wrappers for CEcho. This
		version only prints if the activator is a
		display player.
--------------------------------------------------*/
bool CallFunc_EndPrint(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	auto& info = static_cast<Thread*>(thread)->info;

	if (P_MobjWasRemoved(info.mo) == false && info.mo->player != nullptr)
	{
		if (ACS_ActivatorIsLocal(thread))
			HU_DoCEcho(thread->printBuf.data());
	}

	thread->printBuf.drop();

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerCount(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pushes the number of players to ACS.
--------------------------------------------------*/
bool CallFunc_PlayerCount(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	UINT8 numPlayers = 0;
	UINT8 i;

	(void)argV;
	(void)argC;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = NULL;

		if (playeringame[i] == false)
		{
			continue;
		}

		player = &players[i];

		if (player->spectator == true)
		{
			continue;
		}

		numPlayers++;
	}

	thread->dataStk.push(numPlayers);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_GameType(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pushes the current gametype to ACS.
--------------------------------------------------*/
bool CallFunc_GameType(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push(gametype);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_Timer(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pushes leveltime to ACS.
--------------------------------------------------*/
bool CallFunc_Timer(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push(leveltime);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_IsNetworkGame(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pushes netgame status to ACS.
--------------------------------------------------*/
bool CallFunc_IsNetworkGame(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push(netgame);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SectorSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Plays a point sound effect from a sector.
--------------------------------------------------*/
bool CallFunc_SectorSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	ACSVM::MapScope *map = nullptr;
	ACSVM::String *str = nullptr;

	const char *sfxName = nullptr;
	size_t sfxLen = 0;

	sfxenum_t sfxId = sfx_None;
	INT32 vol = 0;
	mobj_t *origin = nullptr;

	(void)argC;

	map = thread->scopeMap;
	str = map->getString(argV[0]);

	sfxName = str->str;
	sfxLen = str->len;

	if (sfxLen > 0)
	{
		bool success = ACS_GetSFXFromString(sfxName, &sfxId);

		if (success == false)
		{
			// Exit early.
			CONS_Alert(CONS_WARNING,
				"Couldn't find sfx named \"%s\" for SectorSound.\n",
				sfxName
			);

			NO_RETURN(thread);

			return false;
		}
	}

	vol = argV[1];

	if (info->sector != nullptr)
	{
		// New to Ring Racers: Use activating sector directly.
		origin = static_cast<mobj_t *>(static_cast<void *>(&info->sector->soundorg));
	}
	else if (info->line != nullptr)
	{
		// Original Hexen behavior: Use line's frontsector.
		origin = static_cast<mobj_t *>(static_cast<void *>(&info->line->frontsector->soundorg));
	}

	S_StartSoundAtVolume(origin, sfxId, vol);

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_AmbientSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Plays a sound effect globally.
--------------------------------------------------*/
bool CallFunc_AmbientSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = nullptr;
	ACSVM::String *str = nullptr;

	const char *sfxName = nullptr;
	size_t sfxLen = 0;

	sfxenum_t sfxId = sfx_None;
	INT32 vol = 0;

	(void)argC;

	map = thread->scopeMap;
	str = map->getString(argV[0]);

	sfxName = str->str;
	sfxLen = str->len;

	if (sfxLen > 0)
	{
		bool success = ACS_GetSFXFromString(sfxName, &sfxId);

		if (success == false)
		{
			// Exit early.
			CONS_Alert(CONS_WARNING,
				"Couldn't find sfx named \"%s\" for AmbientSound.\n",
				sfxName
			);

			NO_RETURN(thread);

			return false;
		}
	}

	vol = argV[1];

	S_StartSoundAtVolume(NULL, sfxId, vol);

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetLineTexture(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Plays a sound effect globally.
--------------------------------------------------*/
enum
{
	SLT_POS_TOP,
	SLT_POS_MIDDLE,
	SLT_POS_BOTTOM
};

bool CallFunc_SetLineTexture(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	mtag_t tag = 0;
	UINT8 sideId = 0;
	UINT8 texPos = 0;

	ACSVM::MapScope *map = NULL;
	ACSVM::String *str = NULL;
	const char *texName = NULL;
	INT32 texId = LUMPERROR;

	INT32 lineId = -1;

	(void)argC;

	tag = argV[0];
	sideId = (argV[1] & 1);
	texPos = argV[2];

	map = thread->scopeMap;
	str = map->getString(argV[3]);
	texName = str->str;

	texId = R_TextureNumForName(texName);

	TAG_ITER_LINES(tag, lineId)
	{
		line_t *line = &lines[lineId];
		side_t *side = NULL;

		if (line->sidenum[sideId] != 0xffff)
		{
			side = &sides[line->sidenum[sideId]];
		}

		if (side == NULL)
		{
			continue;
		}

		switch (texPos)
		{
			case SLT_POS_MIDDLE:
			{
				side->midtexture = texId;
				break;
			}
			case SLT_POS_BOTTOM:
			{
				side->bottomtexture = texId;
				break;
			}
			case SLT_POS_TOP:
			default:
			{
				side->toptexture = texId;
				break;
			}
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetLineSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Changes a linedef's special and arguments.
--------------------------------------------------*/
bool CallFunc_SetLineSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	mtag_t tag = 0;
	INT32 spec = 0;
	size_t numArgs = 0;

	INT32 lineId = -1;

	tag = argV[0];
	spec = argV[1];

	numArgs = std::min(std::max((signed)(argC - 2), 0), NUM_SCRIPT_ARGS);

	TAG_ITER_LINES(tag, lineId)
	{
		line_t *line = &lines[lineId];
		size_t i;

		if (info->line != nullptr && line == info->line)
		{
			continue;
		}

		line->special = spec;

		for (i = 0; i < numArgs; i++)
		{
			line->args[i] = argV[i + 2];
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_ChangeSky(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Changes the map's sky texture.
--------------------------------------------------*/
bool CallFunc_ChangeSky(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	P_SetupLevelSky(argV[0], argV[1]);

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_ThingSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Plays a sound effect for a tagged object.
--------------------------------------------------*/
bool CallFunc_ThingSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	ACSVM::MapScope *map = nullptr;
	ACSVM::String *str = nullptr;

	const char *sfxName = nullptr;
	size_t sfxLen = 0;

	mtag_t tag = 0;
	sfxenum_t sfxId = sfx_None;
	INT32 vol = 0;

	mobj_t *mobj = nullptr;

	(void)argC;

	tag = argV[0];

	map = thread->scopeMap;
	str = map->getString(argV[1]);

	sfxName = str->str;
	sfxLen = str->len;

	if (sfxLen > 0)
	{
		bool success = ACS_GetSFXFromString(sfxName, &sfxId);

		if (success == false)
		{
			// Exit early.
			CONS_Alert(CONS_WARNING,
				"Couldn't find sfx named \"%s\" for ThingSound.\n",
				sfxName
			);

			NO_RETURN(thread);

			return false;
		}
	}

	vol = argV[2];

	while ((mobj = P_FindMobjFromTID(tag, mobj, info->mo)) != nullptr)
	{
		S_StartSoundAtVolume(mobj, sfxId, vol);
	}

	NO_RETURN(thread);

	return false;
}


/*--------------------------------------------------
	bool CallFunc_EndPrintBold(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		One of the ACS wrappers for CEcho. This
		version prints for all players.
--------------------------------------------------*/
bool CallFunc_EndPrintBold(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	HU_DoCEcho(thread->printBuf.data());

	thread->printBuf.drop();

	NO_RETURN(thread);

	return false;
}
/*--------------------------------------------------
	bool CallFunc_PlayerTeam(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's team ID.
--------------------------------------------------*/
bool CallFunc_PlayerTeam(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	UINT8 teamID = 0;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		teamID = info->mo->player->ctfteam;
	}

	thread->dataStk.push(teamID);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerRings(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's ring count.
--------------------------------------------------*/
bool CallFunc_PlayerRings(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	SINT8 rings = 0;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		rings = info->mo->player->rings;
	}

	thread->dataStk.push(rings);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerScore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's score.
--------------------------------------------------*/
bool CallFunc_PlayerScore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	UINT32 score = 0;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		score = info->mo->player->score;
	}

	thread->dataStk.push(score);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerScore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if the activating player is super.
--------------------------------------------------*/
bool CallFunc_PlayerSuper(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	bool super = false;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		super = (info->mo->player->powers[pw_super] != 0);
	}

	thread->dataStk.push(super);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerNumber(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's ID.
--------------------------------------------------*/
bool CallFunc_PlayerNumber(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	INT16 playerID = -1;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		playerID = (info->mo->player - players);
	}

	thread->dataStk.push(playerID);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_ActivatorTID(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating object's TID.
--------------------------------------------------*/
bool CallFunc_ActivatorTID(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	INT16 tid = 0;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false))
	{
		tid = info->mo->tid;
	}

	thread->dataStk.push(tid);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_EndLog(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		One of the ACS wrappers for CONS_Printf.
		This version only prints if the activator
		is a display player.
--------------------------------------------------*/
bool CallFunc_EndLog(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	if (ACS_ActivatorIsLocal(thread))
		CONS_Printf("%s\n", thread->printBuf.data());

	thread->printBuf.drop();

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_strcmp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		ACS wrapper for strcmp.
--------------------------------------------------*/
static int ACS_strcmp(ACSVM::String *a, ACSVM::String *b)
{
	for (char const *sA = a->str, *sB = b->str; ; ++sA, ++sB)
	{
		char cA = *sA, cB = *sB;

		if (cA != cB)
		{
			return (cA < cB) ? -1 : 1;
		}

		if (!cA)
		{
			return 0;
		}
	}
}

bool CallFunc_strcmp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = NULL;

	ACSVM::String *strA = nullptr;
	ACSVM::String *strB = nullptr;

	(void)argC;

	map = thread->scopeMap;

	strA = map->getString(argV[0]);
	strB = map->getString(argV[1]);

	thread->dataStk.push(ACS_strcmp(strA, strB));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_strcasecmp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		ACS wrapper for strcasecmp / stricmp.
--------------------------------------------------*/
static int ACS_strcasecmp(ACSVM::String *a, ACSVM::String *b)
{
	for (char const *sA = a->str, *sB = b->str; ; ++sA, ++sB)
	{
		char cA = std::tolower(*sA), cB = std::tolower(*sB);

		if (cA != cB)
		{
			return (cA < cB) ? -1 : 1;
		}

		if (!cA)
		{
			return 0;
		}
	}
}

bool CallFunc_strcasecmp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = NULL;

	ACSVM::String *strA = nullptr;
	ACSVM::String *strB = nullptr;

	(void)argC;

	map = thread->scopeMap;

	strA = map->getString(argV[0]);
	strB = map->getString(argV[1]);

	thread->dataStk.push(ACS_strcasecmp(strA, strB));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_CountEnemies(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the number of enemies in the tagged sectors.
--------------------------------------------------*/
static bool ACS_EnemyFilter(mobj_t *mo)
{
	return ((mo->flags & (MF_ENEMY|MF_BOSS)) && mo->health > 0);
}

bool CallFunc_CountEnemies(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	mtag_t tag = 0;
	mtag_t tid = 0;
	UINT32 count = 0;

	(void)argC;

	tag = argV[0];
	tid = argV[1];
	count = ACS_SectorTagThingCounter(tag, info->sector, tid, ACS_EnemyFilter);

	thread->dataStk.push(count);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_CountPushables(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the number of pushables in the tagged sectors.
--------------------------------------------------*/
static bool ACS_PushableFilter(mobj_t *mo)
{
	return ((mo->flags & MF_PUSHABLE)
		|| ((mo->info->flags & MF_PUSHABLE) && mo->fuse));
}

bool CallFunc_CountPushables(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	mtag_t tag = 0;
	mtag_t tid = 0;
	UINT32 count = 0;

	(void)argC;

	tag = argV[0];
	tid = argV[1];
	count = ACS_SectorTagThingCounter(tag, info->sector, tid, ACS_PushableFilter);

	thread->dataStk.push(count);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_HasUnlockable(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if something was unlocked.
--------------------------------------------------*/
bool CallFunc_HasUnlockable(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	UINT32 id = 0;
	bool unlocked = false;

	(void)argC;

	id = argV[0];

	if (id >= MAXUNLOCKABLES)
	{
		CONS_Alert(CONS_WARNING, "HasUnlockable: Bad unlockable ID %d.\n", id);
	}
	else
	{
		unlocked = M_CheckNetUnlockByID(id);
	}

	thread->dataStk.push(unlocked);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SkinUnlocked(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Checks if a certain skin has been unlocked.
--------------------------------------------------*/
bool CallFunc_SkinUnlocked(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	bool unlocked = false;
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		INT32 skinNum;
		if (ACS_GetSkinFromString(thread->scopeMap->getString( argV[0] )->str, &skinNum) == true)
		{
			unlocked = R_SkinUsable(info->mo->player - players, skinNum);
		}
	}

	thread->dataStk.push(unlocked);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerSkin(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's skin name.
--------------------------------------------------*/
bool CallFunc_PlayerSkin(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	Environment *env = &ACSEnv;
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		UINT8 skin = info->mo->player->skin;
		thread->dataStk.push(~env->getString( skins[skin]->name )->idx);
		return false;
	}

	NO_RETURN(thread);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the given player's bot status.
--------------------------------------------------*/
bool CallFunc_PlayerBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	INT32 playernum = argV[0];

	bool isBot = false;

	if (playernum < 0 || playernum >= MAXPLAYERS)
	{
		CONS_Alert(CONS_WARNING, "PlayerBot player %d out of range (expected 0 - %d).\n", playernum, MAXPLAYERS-1);
	}
	else
	{
		if (players[playernum].bot != BOT_NONE)
			isBot = true;
	}

	thread->dataStk.push(isBot);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerExiting(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's exiting status.
--------------------------------------------------*/
bool CallFunc_PlayerExiting(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	INT32 playernum = argV[0];

	bool exiting = false;

	if (playernum < 0 || playernum >= MAXPLAYERS)
	{
		CONS_Alert(CONS_WARNING, "PlayerExiting player %d out of range (expected 0 - %d).\n", playernum, MAXPLAYERS-1);
	}
	else
	{
		if (players[playernum].exiting != 0)
			exiting = true;
	}

	thread->dataStk.push(exiting);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_Teleport(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Teleports the activating actor.
--------------------------------------------------*/
bool CallFunc_Teleport(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, NULL);
	mobj_t *dest = P_FindMobjFromTID(argV[1], NULL, NULL);

	if (mobj != NULL && dest != mobj)
	{
		boolean silent = argC >= 3 ? (argV[2] != 0) : false;

		P_Teleport(mobj, dest->x, dest->y, dest->z, dest->angle, !silent, false);

		if (!silent)
			S_StartSound(dest, sfx_mixup); // Play the 'bowrwoosh!' sound
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetViewpoint(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Switches the current viewpoint to another actor.
--------------------------------------------------*/
bool CallFunc_SetViewpoint(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	if ((info != NULL) && (info->mo != NULL && P_MobjWasRemoved(info->mo) == false))
	{
		mobj_t *altview = P_FindMobjFromTID(argV[0], NULL, info->mo);
		if (!altview)
		{
			NO_RETURN(thread);

			return false;
		}

		// If titlemap, set the camera ref for title's thinker
		// This is not revoked until overwritten; awayviewtics is ignored
		if (titlemapinaction)
		{
			titlemapcameraref = altview;
			titlemapcameraref->cusval = altview->pitch;
		}
		else
		{
			player_t *player = info->mo->player;
			if (!player)
			{
				NO_RETURN(thread);

				return false;
			}

			if (!player->awayviewtics || player->awayviewmobj != altview)
			{
				P_SetTarget(&player->awayviewmobj, altview);

				if (player == &players[displayplayer])
					P_ResetCamera(player, &camera); // reset p1 camera on p1 getting an awayviewmobj
				else if (splitscreen && player == &players[secondarydisplayplayer])
					P_ResetCamera(player, &camera2);  // reset p2 camera on p2 getting an awayviewmobj
			}

			player->awayviewaiming = altview->pitch;
			player->awayviewtics = argC > 1 ? argV[1] : -1;
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	static bool ACS_SpawnObject(player_t *player, INT32 property, const ACSVM::Word *argV, ACSVM::Word argC)

		Helper function for CallFunc_SpawnObject and CallFunc_SpawnObjectForced.
--------------------------------------------------*/
static bool ACS_SpawnObject(mobjtype_t mobjType, bool forceSpawn, const ACSVM::Word *argV, ACSVM::Word argC)
{
	fixed_t x = argV[1];
	fixed_t y = argV[2];
	fixed_t z = argV[3];

	mobj_t *mobj = P_SpawnMobj(x, y, z, mobjType);

	if (P_MobjWasRemoved(mobj))
	{
		return false;
	}
	else if (!forceSpawn && ACS_IsLocationValid(mobj, x, y, z) == false)
	{
		P_RemoveMobj(mobj);

		return false;
	}
	else
	{
		// Spawned successfully
		if (argC >= 5)
			P_SetThingTID(mobj, argV[4]);

		if (argC >= 6)
			mobj->angle = ACS_FixedToAngle(argV[5]);
	}

	return true;
}

/*--------------------------------------------------
	bool CallFunc_SpawnObject(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Spawns an actor.
--------------------------------------------------*/
bool CallFunc_SpawnObject(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::String *str = thread->scopeMap->getString( argV[0] );
	if (!str->str || str->len == 0)
	{
		CONS_Alert(CONS_WARNING, "Spawn actor class was not provided.\n");

		thread->dataStk.push(0);

		return false;
	}

	const char *className = str->str;

	mobjtype_t mobjType = MT_NULL;

	int numSpawned = 0;

	if (ACS_GetMobjTypeFromString(className, &mobjType) == false)
	{
		CONS_Alert(CONS_WARNING,
			"Couldn't find actor class \"%s\" for Spawn.\n",
			className
		);
	}
	else
	{
		if (ACS_SpawnObject(mobjType, false, argV, argC) == true)
			numSpawned = 1;
	}

	thread->dataStk.push(numSpawned);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_SpawnObjectForced(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Spawns an actor, even in locations where it would not normally be able to exist.
--------------------------------------------------*/
bool CallFunc_SpawnObjectForced(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::String *str = thread->scopeMap->getString( argV[0] );
	if (!str->str || str->len == 0)
	{
		CONS_Alert(CONS_WARNING, "SpawnForced actor class was not provided.\n");

		thread->dataStk.push(0);

		return false;
	}

	const char *className = str->str;

	mobjtype_t mobjType = MT_NULL;

	int numSpawned = 0;

	if (ACS_GetMobjTypeFromString(className, &mobjType) == false)
	{
		CONS_Alert(CONS_WARNING,
			"Couldn't find actor class \"%s\" for SpawnForced.\n",
			className
		);
	}
	else
	{
		if (ACS_SpawnObject(mobjType, true, argV, argC) == true)
			numSpawned = 1;
	}

	thread->dataStk.push(numSpawned);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_SpawnProjectile(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Spawns a projectile.
--------------------------------------------------*/
bool CallFunc_SpawnProjectile(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		ACSVM::String *str = thread->scopeMap->getString( argV[1] );
		if (!str->str || str->len == 0)
		{
			CONS_Alert(CONS_WARNING, "SpawnProjectile projectile class was not provided.\n");

			NO_RETURN(thread);

			return false;
		}

		const char *className = str->str;

		mobjtype_t mobjType = MT_NULL;

		if (ACS_GetMobjTypeFromString(className, &mobjType) == false)
		{
			CONS_Alert(CONS_WARNING,
				"Couldn't find actor class \"%s\" for SpawnProjectile.\n",
				className
			);

			NO_RETURN(thread);

			return false;
		}

		mobj_t *missile = P_SpawnMissileAtSpeeds(mobj, mobjType, ACS_FixedToAngle(argV[2]), argV[3], argV[4], argV[5] != 0);
		if (missile && argV[6] != 0)
		{
			P_SetThingTID(mobj, argV[6]);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_TrackObjectAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Implementation of linedef type 457.
--------------------------------------------------*/
bool CallFunc_TrackObjectAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, NULL);
	mobj_t *anchormo = P_FindMobjFromTID(argV[1], NULL, NULL);
	if (mobj != NULL && anchormo != NULL && P_MobjWasRemoved(mobj) == false && P_MobjWasRemoved(anchormo) == false)
	{
		angle_t failureangle = ACS_FixedToAngle(argV[2]);
		INT32 failureexectag = argV[3];
		INT32 failuredelay = argC >= 5 ? abs((int)argV[4]) : 0;
		boolean persist = argC >= 6 ? (argV[5] == 0) : false;

		mobj->eflags |= MFE_TRACERANGLE;
		P_SetTarget(&mobj->tracer, anchormo);
		mobj->lastlook = persist; // don't disable behavior after first failure
		mobj->extravalue1 = failureangle; // angle to exceed for failure state
		mobj->extravalue2 = failureexectag; // exec tag for failure state (angle is not within range)
		mobj->cusval = mobj->cvmem = failuredelay; // cusval = tics to allow failure before line trigger; cvmem = decrement timer
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_StopTrackingObjectAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Implementation of linedef type 458.
--------------------------------------------------*/
bool CallFunc_StopTrackingObjectAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, NULL);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false && (mobj->eflags & MFE_TRACERANGLE))
	{
		mobj->eflags &= ~MFE_TRACERANGLE;
		P_SetTarget(&mobj->tracer, NULL);
		mobj->lastlook = mobj->cvmem = mobj->cusval = mobj->extravalue1 = mobj->extravalue2 = 0;
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_Check2DMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Checks if the activator is in 2D mode.
--------------------------------------------------*/
bool CallFunc_Check2DMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	bool in2DMode = false;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false))
	{
		if (info->mo->flags2 & MF2_TWOD)
			in2DMode = true;
	}

	thread->dataStk.push(in2DMode);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_Set2DMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Implementation of linedef type 432.
--------------------------------------------------*/
bool CallFunc_Set2DMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false))
	{
		if (argV[0] != 0)
			info->mo->flags2 |= MF2_TWOD;
		else
			info->mo->flags2 &= ~MF2_TWOD;
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerEmeralds(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's number of Chaos Emeralds.
--------------------------------------------------*/
bool CallFunc_PlayerEmeralds(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	UINT8 count = 0;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		UINT32 emerbits;

		if (G_PlatformGametype())
			emerbits = emeralds;
		else
			emerbits = info->mo->player->powers[pw_emeralds];

		for (unsigned i = 0; i < 7; i++)
		{
			if (emerbits & (1 << i))
				count++;
		}
	}

	thread->dataStk.push(count);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerLap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's current lap.
--------------------------------------------------*/
bool CallFunc_PlayerLap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	UINT8 laps = 0;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		laps = info->mo->player->laps;
	}

	thread->dataStk.push(laps);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_LowestLap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the lowest lap of all of the players in-game.
--------------------------------------------------*/
bool CallFunc_LowestLap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push(P_FindLowestLap());
	return false;
}

/*--------------------------------------------------
	bool CallFunc_RingslingerMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if the map is currently in a shooting gametype.
--------------------------------------------------*/
bool CallFunc_RingSlingerMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push(G_RingSlingerGametype());
	return false;
}

/*--------------------------------------------------
	bool CallFunc_CaptureTheFlagMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if the map is currently in a Capture The Flag gametype.
--------------------------------------------------*/
bool CallFunc_CaptureTheFlagMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push((gametyperules & GTR_TEAMFLAGS) != 0);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_TeamGame(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if the map is currently in a team gametype.
--------------------------------------------------*/
bool CallFunc_TeamGame(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push(G_GametypeHasTeams());
	return false;
}

/*--------------------------------------------------
	bool CallFunc_ModeAttacking(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if the map is a Mode Attack session.
--------------------------------------------------*/
bool CallFunc_ModeAttacking(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push((modeattacking != ATTACKING_NONE));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_RecordAttack(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if the map is a Record Attack session.
--------------------------------------------------*/
bool CallFunc_RecordAttack(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push((modeattacking == ATTACKING_RECORD));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_NiGHTSAttack(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if the map is a NiGHTS Attack session.
--------------------------------------------------*/
bool CallFunc_NiGHTSAttack(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push((modeattacking == ATTACKING_NIGHTS));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetLineRenderStyle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Changes a linedef's blend mode and alpha.
--------------------------------------------------*/
bool CallFunc_SetLineRenderStyle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	mtag_t tag = 0;
	patchalphastyle_t blend = AST_COPY;
	fixed_t alpha = FRACUNIT;

	INT32 lineId = -1;

	(void)argC;

	tag = argV[0];

	switch (argV[1])
	{
		case TMB_TRANSLUCENT:
		default:
			blend = AST_COPY;
			break;
		case TMB_ADD:
			blend = AST_ADD;
			break;
		case TMB_SUBTRACT:
			blend = AST_SUBTRACT;
			break;
		case TMB_REVERSESUBTRACT:
			blend = AST_REVERSESUBTRACT;
			break;
		case TMB_MODULATE:
			blend = AST_MODULATE;
			break;
	}

	alpha = argV[2];
	alpha = std::clamp(alpha, 0, FRACUNIT);

	TAG_ITER_LINES(tag, lineId)
	{
		line_t *line = &lines[lineId];

		line->blendmode = blend;
		line->alpha = alpha;
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectX(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the X position of an actor.
--------------------------------------------------*/
bool CallFunc_GetObjectX(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	fixed_t value = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		value = mobj->x;
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectY(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the Y position of an actor.
--------------------------------------------------*/
bool CallFunc_GetObjectY(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	fixed_t value = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		value = mobj->y;
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectZ(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the Z position of an actor.
--------------------------------------------------*/
bool CallFunc_GetObjectZ(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	fixed_t value = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		value = mobj->z;
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectVelX(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the X velocity of an actor.
--------------------------------------------------*/
bool CallFunc_GetObjectVelX(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	fixed_t value = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		value = mobj->momx;
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectVelY(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the Y velocity of an actor.
--------------------------------------------------*/
bool CallFunc_GetObjectVelY(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	fixed_t value = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		value = mobj->momy;
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectVelZ(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the Z velocity of an actor.
--------------------------------------------------*/
bool CallFunc_GetObjectVelZ(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	fixed_t value = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		value = mobj->momz;
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the yaw of an actor.
--------------------------------------------------*/
bool CallFunc_GetObjectAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	fixed_t value = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		value = ACS_AngleToFixed(mobj->angle);
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectRoll(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the roll of an actor.
--------------------------------------------------*/
bool CallFunc_GetObjectRoll(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	fixed_t value = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		value = ACS_AngleToFixed(mobj->roll);
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectPitch(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the pitch of an actor.
--------------------------------------------------*/
bool CallFunc_GetObjectPitch(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	fixed_t value = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		value = ACS_AngleToFixed(mobj->pitch);
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectFloorZ(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the highest floor point underneath the actor.
--------------------------------------------------*/
bool CallFunc_GetObjectFloorZ(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	fixed_t value = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		value = ACS_AngleToFixed(mobj->pitch);
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectCeilingZ(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the lowest ceiling point above the actor.
--------------------------------------------------*/
bool CallFunc_GetObjectCeilingZ(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	fixed_t value = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		value = mobj->ceilingz;
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectFloorTexture(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the texture of the floor where the actor is currently on.
--------------------------------------------------*/
bool CallFunc_GetObjectFloorTexture(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	Environment *env = &ACSEnv;
	auto info = &static_cast<Thread *>(thread)->info;

	INT32 floorpic = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		INT32 picnum = P_FloorPicAtPos(mobj->x, mobj->y, mobj->z, mobj->height);
		if (picnum != -1)
		{
			floorpic = ~env->getString( levelflats[picnum].name )->idx;
		}
	}

	thread->dataStk.push(floorpic);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectLightLevel(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the light level of the place where the actor is currently in.
--------------------------------------------------*/
bool CallFunc_GetObjectLightLevel(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	INT32 lightlevel = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		lightlevel = P_GetLightLevelFromSectorAt(mobj->subsector->sector, mobj->x, mobj->y, mobj->z);
	}

	thread->dataStk.push(lightlevel);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_CheckObjectState(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Checks if the actor with the matching TID is in the specified state. If said TID is zero, this checks the activator.
--------------------------------------------------*/
bool CallFunc_CheckObjectState(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	bool inState = false;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		statenum_t stateNum = S_NULL;
		bool success = ACS_GetStateFromString(thread->scopeMap->getString( argV[1] )->str, &stateNum);
		if (success == true)
		{
			if (mobj->state == &states[stateNum])
			{
				inState = true;
			}
		}
	}

	thread->dataStk.push(inState);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_CheckObjectFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Checks if the actor with the matching TID has a specific flag. If said TID is zero, this checks the activator.
--------------------------------------------------*/
bool CallFunc_CheckObjectFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argC;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);

	const char *flagName = thread->scopeMap->getString( argV[1] )->str;

	bool hasFlag = false;

	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		if (ACS_CheckActorFlag(flagName, NULL, NULL) == true)
		{
			hasFlag = true;
		}
		else
		{
			CONS_Alert(CONS_WARNING, "CheckActorFlag: no actor flag named \"%s\".\n", flagName);
		}
	}

	thread->dataStk.push(hasFlag);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectClass(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the class name of the actor.
--------------------------------------------------*/
bool CallFunc_GetObjectClass(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	Environment *env = &ACSEnv;
	auto info = &static_cast<Thread *>(thread)->info;

	INT32 mobjClass = 0;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);
	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		if (mobj->type >= MT_FIRSTFREESLOT)
		{
			std::string	prefix = "MT_";
			std::string	full = prefix + FREE_MOBJS[mobj->type - MT_FIRSTFREESLOT];
			mobjClass = static_cast<INT32>( ~env->getString( full.c_str() )->idx );
		}
		else
		{
			mobjClass = static_cast<INT32>( ~env->getString( MOBJTYPE_LIST[ mobj->type ] )->idx );
		}
	}

	thread->dataStk.push(mobjClass);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the actor's dye.
--------------------------------------------------*/
bool CallFunc_GetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	Environment *env = &ACSEnv;
	auto info = &static_cast<Thread *>(thread)->info;
	UINT16 dye = SKINCOLOR_NONE;

	(void)argC;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);

	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		dye = (mobj->player != NULL) ? mobj->player->powers[pw_dye] : mobj->color;
	}

	thread->dataStk.push(~env->getString( skincolors[dye].name )->idx);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetObjectPosition(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Sets the position of the actor with the matching TID. If said TID is zero, this affects the activator.
--------------------------------------------------*/
bool CallFunc_SetObjectPosition(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	// SetActorPosition's signature makes the 'fog' parameter required. Sad!
	// So we just ignore it.
	auto info = &static_cast<Thread *>(thread)->info;
	bool success = false;

	(void)argC;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);

	fixed_t x = argV[1];
	fixed_t y = argV[2];
	fixed_t z = argV[3];

	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		if (ACS_IsLocationValid(mobj, x, y, z))
		{
			P_SetOrigin(mobj, x, y, z);
			success = true;
		}
	}

	thread->dataStk.push(success);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetObjectVelocity(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Sets the velocity of every actor with the matching TID. If said TID is zero, this affects the activator.
--------------------------------------------------*/
bool CallFunc_SetObjectVelocity(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	fixed_t velX = argV[1];
	fixed_t velY = argV[2];
	fixed_t velZ = argV[3];

	bool add = argC >= 5 ? (argV[4] != 0) : false;

	mobj_t *mobj = nullptr;

	while ((mobj = P_FindMobjFromTID(argV[0], mobj, info->mo)) != nullptr)
	{
		if (add)
		{
			mobj->momx += velX;
			mobj->momy += velY;
			mobj->momz += velZ;
		}
		else
		{
			mobj->momx = velX;
			mobj->momy = velY;
			mobj->momz = velZ;
		}
	}

	NO_RETURN(thread);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetObjectAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Sets the angle of every actor with the matching TID. If said TID is zero, this affects the activator.
--------------------------------------------------*/
bool CallFunc_SetObjectAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argC;

	angle_t angle = ACS_FixedToAngle(argV[1]);

	mobj_t *mobj = nullptr;

	while ((mobj = P_FindMobjFromTID(argV[0], mobj, info->mo)) != nullptr)
	{
		mobj->angle = angle;
	}

	NO_RETURN(thread);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetObjectRoll(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Sets the roll of every actor with the matching TID. If said TID is zero, this affects the activator.
--------------------------------------------------*/
bool CallFunc_SetObjectRoll(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argC;

	angle_t roll = ACS_FixedToAngle(argV[1]);

	mobj_t *mobj = nullptr;

	while ((mobj = P_FindMobjFromTID(argV[0], mobj, info->mo)) != nullptr)
	{
		mobj->roll = roll;
	}

	NO_RETURN(thread);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetObjectPitch(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Sets the pitch of every actor with the matching TID. If said TID is zero, this affects the activator.
--------------------------------------------------*/
bool CallFunc_SetObjectPitch(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argC;

	angle_t pitch = ACS_FixedToAngle(argV[1]);

	mobj_t *mobj = nullptr;

	while ((mobj = P_FindMobjFromTID(argV[0], mobj, info->mo)) != nullptr)
	{
		mobj->pitch = pitch;
	}

	NO_RETURN(thread);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetObjectState(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Sets the state of every actor with the matching TID. If said TID is zero, this affects the activator.
--------------------------------------------------*/
bool CallFunc_SetObjectState(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argC;

	int count = 0;

	mobj_t *mobj = nullptr;

	while ((mobj = P_FindMobjFromTID(argV[0], mobj, info->mo)) != nullptr)
	{
		statenum_t newState = S_NULL;
		bool success = ACS_GetStateFromString(thread->scopeMap->getString( argV[1] )->str, &newState);
		if (success == true)
		{
			P_SetMobjState(mobj, newState);
			count++;
		}
	}

	thread->dataStk.push(count);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetObjectFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Toggles a flag for every actor with the matching TID. If said TID is zero, this affects the activator.
--------------------------------------------------*/
bool CallFunc_SetObjectFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argC;

	int count = 0;

	mobj_t *mobj = nullptr;

	const char *flagName = thread->scopeMap->getString( argV[1] )->str;

	bool add = !!argV[2];

	while ((mobj = P_FindMobjFromTID(argV[0], mobj, info->mo)) != nullptr)
	{
		UINT32 flags = 0;
		unsigned type = 0;

		if (ACS_CheckActorFlag(flagName, &flags, &type) == false)
		{
			CONS_Alert(CONS_WARNING, "SetActorFlag: no actor flag named \"%s\".\n", flagName);
			break;
		}

		// flags
		if (type == 1)
		{
			if (add)
			{
				if ((flags & (MF_NOBLOCKMAP|MF_NOSECTOR)) != (mobj->flags & (MF_NOBLOCKMAP|MF_NOSECTOR)))
				{
					P_UnsetThingPosition(mobj);
					mobj->flags |= static_cast<mobjflag_t>(flags);
					if ((flags & MF_NOSECTOR) && sector_list)
					{
						P_DelSeclist(sector_list);
						sector_list = NULL;
					}
					mobj->snext = NULL, mobj->sprev = NULL;
					P_SetThingPosition(mobj);
				}
				else
				{
					mobj->flags |= static_cast<mobjflag_t>(flags);
				}
			}
			else
			{
				mobj->flags &= static_cast<mobjflag_t>(~flags);
			}
		}
		// flags2
		else if (type == 2)
		{
			if (add)
				mobj->flags2 |= static_cast<mobjflag2_t>(flags);
			else
				mobj->flags2 &= static_cast<mobjflag2_t>(~flags);
		}
		// eflags
		else if (type == 3)
		{
			if (add)
				mobj->eflags |= static_cast<mobjeflag_t>(flags);
			else
				mobj->eflags &= static_cast<mobjeflag_t>(~flags);
		}

		count++;
	}

	thread->dataStk.push(count);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetObjectClass(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Sets the class of the actor with the matching TID. If said TID is zero, this affects the activator.
--------------------------------------------------*/
bool CallFunc_SetObjectClass(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argC;

	ACSVM::String *str = thread->scopeMap->getString( argV[1] );
	if (!str->str || str->len == 0)
	{
		CONS_Alert(CONS_WARNING, "SetActorClass actor class was not provided.\n");

		NO_RETURN(thread);

		return false;
	}

	const char *className = str->str;

	mobjtype_t mobjType = MT_NULL;

	mobj_t *mobj = P_FindMobjFromTID(argV[0], NULL, info->mo);

	if (mobj != NULL && P_MobjWasRemoved(mobj) == false)
	{
		bool success = ACS_GetMobjTypeFromString(className, &mobjType);

		if (success == false)
		{
			// Exit early.
			CONS_Alert(CONS_WARNING,
				"Couldn't find actor class \"%s\" for SetActorClass.\n",
				className
			);

			NO_RETURN(thread);

			return false;
		}

		mobj->type = mobjType;
		mobj->info = &mobjinfo[mobjType];
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Dyes every actor with the matching TID. If said TID is zero, this affects the activator.
--------------------------------------------------*/
bool CallFunc_SetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	ACSVM::String *str = thread->scopeMap->getString(argV[1]);

	skincolornum_t colorToSet = SKINCOLOR_NONE;

	if (str->len > 0)
	{
		const char *colorName = str->str;

		bool success = ACS_GetColorFromString(colorName, &colorToSet);

		if (success == false)
		{
			// Exit early.
			CONS_Alert(CONS_WARNING,
				"Couldn't find color \"%s\" for SetActorDye.\n",
				colorName
			);

			NO_RETURN(thread);

			return false;
		}
	}
	else
	{
		CONS_Alert(CONS_WARNING, "SetActorDye color was not provided.\n");

		NO_RETURN(thread);

		return false;
	}

	mobj_t *mobj = nullptr;

	while ((mobj = P_FindMobjFromTID(argV[0], mobj, info->mo)) != nullptr)
	{
		var1 = 0;
		var2 = colorToSet;
		A_Dye(info->mo);
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetObjectSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Sets the special and arguments of every actor with the matching TID. If said TID is zero, this affects the activator.
--------------------------------------------------*/
bool CallFunc_SetObjectSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = thread->scopeMap;

	auto info = &static_cast<Thread *>(thread)->info;

	mobj_t *mobj = nullptr;

	while ((mobj = P_FindMobjFromTID(argV[0], mobj, info->mo)) != nullptr)
	{
		mobj->special = argV[1];

		for (int i = 0; i < std::min((int)(argC - 2), NUM_SCRIPT_ARGS); i++)
		{
			mobj->script_args[i] = (INT32)argV[i + 2];
		}

		for (int i = 0; i < std::min((int)(argC - 2), NUM_SCRIPT_STRINGARGS); i++)
		{
			ACSVM::String *strPtr = map->getString(argV[i + 2]);

			size_t len = strPtr->len;

			if (len == 0)
			{
				Z_Free(mobj->script_stringargs[i]);
				mobj->script_stringargs[i] = NULL;
				continue;
			}

			mobj->script_stringargs[i] = static_cast<char *>(Z_Realloc(mobj->script_stringargs[i], len + 1, PU_STATIC, nullptr));
			M_Memcpy(mobj->script_stringargs[i], strPtr->str, len + 1);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_MapWarp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Immediately warps to another level.
--------------------------------------------------*/

bool CallFunc_MapWarp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = NULL;

	ACSVM::String *str = nullptr;

	const char *levelName = NULL;
	size_t levelLen = 0;

	(void)argC;

	map = thread->scopeMap;

	str = map->getString(argV[0]);

	levelName = str->str;
	levelLen = str->len;

	if (!levelLen || !levelName)
	{
		CONS_Alert(CONS_WARNING, "MapWarp level name was not provided.\n");
	}
	else
	{
		INT16 nextmap = 0;

		if (levelName[0] == 'M' && levelName[1] == 'A' && levelName[2] == 'P' && levelName[5] == '\0')
		{
			nextmap = (INT16)M_MapNumber(levelName[3], levelName[4]);
		}

		if (nextmap == 0)
		{
			CONS_Alert(CONS_WARNING, "MapWarp level %s is not valid or loaded.\n", levelName);
		}
		else
		{
			nextmapoverride = nextmap;

			if (argV[1] == 0)
				skipstats = 1;

			if (server)
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_AddBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Inserts a bot, if there's room for them.
--------------------------------------------------*/
bool CallFunc_AddBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = thread->scopeMap;

	ACSVM::String *skinStr = nullptr;
	const char *skinname = NULL;

	ACSVM::String *nameStr = nullptr;
	const char *botname = NULL;

	skincolornum_t skincolor = SKINCOLOR_NONE;

	SINT8 bottype = BOT_MPAI;

	// Get name
	skinStr = map->getString(argV[0]);
	if (skinStr->len != 0)
		skinname = skinStr->str;

	// Get skincolor
	if (argC >= 2)
	{
		ACSVM::String *colorStr = thread->scopeMap->getString(argV[1]);

		const char *colorName = colorStr->str;

		bool success = ACS_GetColorFromString(colorName, &skincolor);

		if (success == false)
		{
			// Exit early.
			CONS_Alert(CONS_WARNING,
				"Couldn't find color \"%s\" for AddBot.\n",
				colorName
			);

			NO_RETURN(thread);

			return false;
		}
	}

	// Get type
	if (argC >= 3)
	{
		bottype = static_cast<int>(argV[2]);
		if (bottype < BOT_NONE || bottype > BOT_MPAI)
		{
			bottype = BOT_MPAI;
		}
	}

	// Get name
	if (argC >= 3)
	{
		nameStr = map->getString(argV[3]);
		if (nameStr->len != 0)
		{
			botname = nameStr->str;
		}
	}

	thread->dataStk.push(B_AddBot(skinname, skincolor, botname, bottype));

	return false;
}

/*--------------------------------------------------
	bool CallFunc_RemoveBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Removes a bot.
--------------------------------------------------*/
bool CallFunc_RemoveBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	int playernum = argV[0];

	if (playernum < 0 || playernum >= MAXPLAYERS)
	{
		CONS_Alert(CONS_WARNING, "RemoveBot player %d out of range (expected 0 - %d).\n", playernum, MAXPLAYERS-1);
	}
	else if (playeringame[playernum])
	{
		if (players[playernum].bot == BOT_NONE)
		{
			CONS_Alert(CONS_WARNING, "RemoveBot cannot remove human\n");
		}
		else
		{
			players[playernum].removing = true;
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_ExitLevel(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Exits the level.
--------------------------------------------------*/
bool CallFunc_ExitLevel(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	if (argC >= 1)
	{
		skipstats = (argV[0] == 0);
	}

	if (server)
		SendNetXCmd(XD_EXITLEVEL, NULL, 0);

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_MusicPlay(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Play a tune. If it's already playing, restart from the
		beginning.
--------------------------------------------------*/
bool CallFunc_MusicPlay(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = thread->scopeMap;

	// 0: str tune - id for the tune to play
	// 1: [bool foractivator] - only do this if the activator is a player and is being viewed

	if (argC > 1 && argV[1] && !ACS_ActivatorIsLocal(thread))
	{
		NO_RETURN(thread);

		return false;
	}

	S_StopMusic();
	S_ChangeMusicInternal(map->getString(argV[0])->str, true);

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_MusicStopAll(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Stop every tune that is currently playing.
--------------------------------------------------*/
bool CallFunc_MusicStopAll(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	// 0: [bool foractivator] - only do this if the activator is a player and is being viewed

	if (argC > 0 && argV[0] && !ACS_ActivatorIsLocal(thread))
	{
		NO_RETURN(thread);

		return false;
	}

	S_StopMusic();

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_MusicRestore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Restores the map music.
--------------------------------------------------*/
bool CallFunc_MusicRestore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	S_ChangeMusicEx(mapmusname, mapmusflags, true, mapmusposition, 0, 0);

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_MusicDim(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Fade level music into or out of silence.
--------------------------------------------------*/
bool CallFunc_MusicDim(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	// 0: int fade time (ms) - time to fade between full volume and silence
	UINT32 fade = argV[0];

	S_FadeOutStopMusic(fade);

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_Get/SetLineProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Generic line property management.
--------------------------------------------------*/
enum
{
	LINE_PROP_FLAGS,
	LINE_PROP_ALPHA,
	LINE_PROP_BLENDMODE,
	LINE_PROP_ACTIVATION,
	LINE_PROP_ACTION,
	LINE_PROP_ARG0,
	LINE_PROP_ARG1,
	LINE_PROP_ARG2,
	LINE_PROP_ARG3,
	LINE_PROP_ARG4,
	LINE_PROP_ARG5,
	LINE_PROP_ARG6,
	LINE_PROP_ARG7,
	LINE_PROP_ARG8,
	LINE_PROP_ARG9,
	LINE_PROP_ARG0STR,
	LINE_PROP_ARG1STR,
	LINE_PROP__MAX
};

static INT32 NextLine(mtag_t tag, size_t *iterate, INT32 activatorID)
{
	size_t i = *iterate;
	*iterate = *iterate + 1;

	if (tag == 0)
	{
		// 0 grabs the activator.

		if (i != 0)
		{
			// Don't do more than once.
			return -1;
		}

		return activatorID;
	}

	return Tag_Iterate_Lines(tag, i);
}

bool CallFunc_GetLineProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;
	Environment *env = &ACSEnv;

	mtag_t tag = 0;
	size_t tagIt = 0;

	INT32 lineID = 0;
	INT32 activatorID = -1;
	line_t *line = NULL;

	INT32 property = LINE_PROP__MAX;
	INT32 value = 0;

	tag = argV[0];

	if (info != NULL && info->line != NULL)
	{
		activatorID = info->line - lines;
	}

	if ((lineID = NextLine(tag, &tagIt, activatorID)) != -1)
	{
		line = &lines[ lineID ];
	}

	property = argV[1];

	if (line != NULL)
	{

#define PROP_INT(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( line->y ); \
		break; \
	}

#define PROP_STR(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( ~env->getString( line->y )->idx ); \
		break; \
	}

		switch (property)
		{
			PROP_INT(LINE_PROP_FLAGS, flags)
			PROP_INT(LINE_PROP_ALPHA, alpha)
			PROP_INT(LINE_PROP_BLENDMODE, blendmode)
			PROP_INT(LINE_PROP_ACTIVATION, activation)
			PROP_INT(LINE_PROP_ACTION, special)
			PROP_INT(LINE_PROP_ARG0, args[0])
			PROP_INT(LINE_PROP_ARG1, args[1])
			PROP_INT(LINE_PROP_ARG2, args[2])
			PROP_INT(LINE_PROP_ARG3, args[3])
			PROP_INT(LINE_PROP_ARG4, args[4])
			PROP_INT(LINE_PROP_ARG5, args[5])
			PROP_INT(LINE_PROP_ARG6, args[6])
			PROP_INT(LINE_PROP_ARG7, args[7])
			PROP_INT(LINE_PROP_ARG8, args[8])
			PROP_INT(LINE_PROP_ARG9, args[9])
			PROP_STR(LINE_PROP_ARG0STR, stringargs[0])
			PROP_STR(LINE_PROP_ARG1STR, stringargs[1])
			default:
			{
				CONS_Alert(CONS_WARNING, "GetLineProperty type %d out of range (expected 0 - %d).\n", property, LINE_PROP__MAX-1);
				break;
			}
		}

#undef PROP_STR
#undef PROP_INT

	}

	thread->dataStk.push(value);
	return false;
}

bool CallFunc_SetLineProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;
	//Environment *env = &ACSEnv;

	mtag_t tag = 0;
	size_t tagIt = 0;

	INT32 lineID = 0;
	INT32 activatorID = -1;
	line_t *line = NULL;

	INT32 property = LINE_PROP__MAX;
	INT32 value = 0;

	tag = argV[0];

	if (info != NULL && info->line != NULL)
	{
		activatorID = info->line - lines;
	}

	if ((lineID = NextLine(tag, &tagIt, activatorID)) != -1)
	{
		line = &lines[ lineID ];
	}

	property = argV[1];
	value = argV[2];

	while (line != NULL)
	{

#define PROP_READONLY(x, y) \
	case x: \
	{ \
		CONS_Alert(CONS_WARNING, "SetLineProperty type '%s' cannot be written to.\n", "y"); \
		break; \
	}

#define PROP_INT(x, y) \
	case x: \
	{ \
		line->y = static_cast< decltype(line->y) >(value); \
		break; \
	}

#define PROP_STR(x, y) \
	case x: \
	{ \
		ACSVM::String *str = thread->scopeMap->getString( value ); \
		if (str->len == 0) \
		{ \
			Z_Free(line->y); \
			line->y = NULL; \
		} \
		else \
		{ \
			line->y = static_cast<char *>(Z_Realloc(line->y, str->len + 1, PU_LEVEL, NULL)); \
			M_Memcpy(line->y, str->str, str->len + 1); \
			line->y[str->len] = '\0'; \
		} \
		break; \
	}

		switch (property)
		{
			PROP_INT(LINE_PROP_FLAGS, flags)
			PROP_INT(LINE_PROP_ALPHA, alpha)
			PROP_INT(LINE_PROP_BLENDMODE, blendmode)
			PROP_INT(LINE_PROP_ACTIVATION, activation)
			PROP_INT(LINE_PROP_ACTION, special)
			PROP_INT(LINE_PROP_ARG0, args[0])
			PROP_INT(LINE_PROP_ARG1, args[1])
			PROP_INT(LINE_PROP_ARG2, args[2])
			PROP_INT(LINE_PROP_ARG3, args[3])
			PROP_INT(LINE_PROP_ARG4, args[4])
			PROP_INT(LINE_PROP_ARG5, args[5])
			PROP_INT(LINE_PROP_ARG6, args[6])
			PROP_INT(LINE_PROP_ARG7, args[7])
			PROP_INT(LINE_PROP_ARG8, args[8])
			PROP_INT(LINE_PROP_ARG9, args[9])
			PROP_STR(LINE_PROP_ARG0STR, stringargs[0])
			PROP_STR(LINE_PROP_ARG1STR, stringargs[1])
			default:
			{
				CONS_Alert(CONS_WARNING, "SetLineProperty type %d out of range (expected 0 - %d).\n", property, LINE_PROP__MAX-1);
				break;
			}
		}

		if ((lineID = NextLine(tag, &tagIt, activatorID)) != -1)
		{
			line = &lines[ lineID ];
		}
		else
		{
			line = NULL;
		}

#undef PROP_STR
#undef PROP_INT
#undef PROP_READONLY

	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_Get/SetSideProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Generic side property management.
--------------------------------------------------*/
enum
{
	SIDE_FRONT = 0,
	SIDE_BACK = 1,
	SIDE_BOTH,
};

enum
{
	SIDE_PROP_XOFFSET,
	SIDE_PROP_YOFFSET,
	SIDE_PROP_TOPTEXTURE,
	SIDE_PROP_BOTTOMTEXTURE,
	SIDE_PROP_MIDTEXTURE,
	SIDE_PROP_REPEATCOUNT,
	SIDE_PROP__MAX
};

bool CallFunc_GetSideProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;
	Environment *env = &ACSEnv;

	mtag_t tag = 0;
	size_t tagIt = 0;

	INT32 lineID = 0;
	INT32 activatorID = -1;
	line_t *line = NULL;

	UINT8 sideID = 0;
	side_t *side = NULL;

	INT32 property = SIDE_PROP__MAX;
	INT32 value = 0;

	tag = argV[0];

	if (info != NULL && info->line != NULL)
	{
		activatorID = info->line - lines;
	}

	if ((lineID = NextLine(tag, &tagIt, activatorID)) != -1)
	{
		line = &lines[ lineID ];
	}

	sideID = argV[1];
	switch (sideID)
	{
		default: // Activator
		case SIDE_BOTH: // Wouldn't make sense for this function.
		{
			sideID = info->side;
			break;
		}
		case SIDE_FRONT:
		case SIDE_BACK:
		{
			// Keep sideID as is.
			break;
		}
	}

	if (line != NULL && line->sidenum[sideID] != 0xffff)
	{
		side = &sides[line->sidenum[sideID]];
	}

	property = argV[2];

	if (side != NULL)
	{

#define PROP_INT(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( side->y ); \
		break; \
	}

#define PROP_STR(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( ~env->getString( side->y )->idx ); \
		break; \
	}

#define PROP_TEXTURE(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( ~env->getString( textures[ side->y ]->name )->idx ); \
		break; \
	}

		switch (property)
		{
			PROP_INT(SIDE_PROP_XOFFSET, textureoffset)
			PROP_INT(SIDE_PROP_YOFFSET, rowoffset)
			PROP_TEXTURE(SIDE_PROP_TOPTEXTURE, toptexture)
			PROP_TEXTURE(SIDE_PROP_BOTTOMTEXTURE, bottomtexture)
			PROP_TEXTURE(SIDE_PROP_MIDTEXTURE, midtexture)
			PROP_INT(SIDE_PROP_REPEATCOUNT, repeatcnt)
			default:
			{
				CONS_Alert(CONS_WARNING, "GetSideProperty type %d out of range (expected 0 - %d).\n", property, SIDE_PROP__MAX-1);
				break;
			}
		}

#undef PROP_TEXTURE
#undef PROP_STR
#undef PROP_INT

	}

	thread->dataStk.push(value);
	return false;
}

bool CallFunc_SetSideProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;
	//Environment *env = &ACSEnv;

	mtag_t tag = 0;
	size_t tagIt = 0;

	INT32 lineID = 0;
	INT32 activatorID = -1;
	line_t *line = NULL;

	UINT8 sideID = 0;
	side_t *side = NULL;
	boolean tryBoth = false;

	INT32 property = SIDE_PROP__MAX;
	INT32 value = 0;

	tag = argV[0];

	if (info != NULL && info->line != NULL)
	{
		activatorID = info->line - lines;
	}

	if ((lineID = NextLine(tag, &tagIt, activatorID)) != -1)
	{
		line = &lines[ lineID ];
	}

	sideID = argV[1];
	switch (sideID)
	{
		default: // Activator
		{
			sideID = info->side;
			break;
		}
		case SIDE_BOTH:
		{
			sideID = SIDE_FRONT;
			tryBoth = true;
			break;
		}
		case SIDE_FRONT:
		case SIDE_BACK:
		{
			// Keep sideID as is.
			break;
		}
	}

	if (line != NULL && line->sidenum[sideID] != 0xffff)
	{
		side = &sides[line->sidenum[sideID]];
	}

	property = argV[2];
	value = argV[3];

	while (line != NULL)
	{
		if (side != NULL)
		{

#define PROP_READONLY(x, y) \
	case x: \
	{ \
		CONS_Alert(CONS_WARNING, "SetSideProperty type '%s' cannot be written to.\n", "y"); \
		break; \
	}

#define PROP_INT(x, y) \
	case x: \
	{ \
		side->y = static_cast< decltype(side->y) >(value); \
		break; \
	}

#define PROP_STR(x, y) \
	case x: \
	{ \
		ACSVM::String *str = thread->scopeMap->getString( value ); \
		if (str->len == 0) \
		{ \
			Z_Free(side->y); \
			side->y = NULL; \
		} \
		else \
		{ \
			side->y = static_cast<char *>(Z_Realloc(side->y, str->len + 1, PU_LEVEL, NULL)); \
			M_Memcpy(side->y, str->str, str->len + 1); \
			side->y[str->len] = '\0'; \
		} \
		break; \
	}

#define PROP_TEXTURE(x, y) \
	case x: \
	{ \
		side->y = R_TextureNumForName( thread->scopeMap->getString( value )->str ); \
		break; \
	}

			auto install_interpolator = [side]
			{
				if (side->acs_interpolated)
					return;
				side->acs_interpolated = true;
				R_CreateInterpolator_SideScroll(nullptr, side);
			};

			switch (property)
			{
				case SIDE_PROP_XOFFSET:
				{
					side->textureoffset = static_cast< decltype(side->textureoffset) >(value);
					install_interpolator();
					break;
				}
				case SIDE_PROP_YOFFSET:
				{
					side->rowoffset = static_cast< decltype(side->rowoffset) >(value);
					install_interpolator();
					break;
				}
				PROP_TEXTURE(SIDE_PROP_TOPTEXTURE, toptexture)
				PROP_TEXTURE(SIDE_PROP_BOTTOMTEXTURE, bottomtexture)
				PROP_TEXTURE(SIDE_PROP_MIDTEXTURE, midtexture)
				PROP_INT(SIDE_PROP_REPEATCOUNT, repeatcnt)
				default:
				{
					CONS_Alert(CONS_WARNING, "SetSideProperty type %d out of range (expected 0 - %d).\n", property, SIDE_PROP__MAX-1);
					break;
				}
			}
		}

		if (tryBoth == true && sideID == SIDE_FRONT)
		{
			sideID = SIDE_BACK;

			if (line->sidenum[sideID] != 0xffff)
			{
				side = &sides[line->sidenum[sideID]];
				continue;
			}
		}

		if ((lineID = NextLine(tag, &tagIt, activatorID)) != -1)
		{
			line = &lines[ lineID ];

			if (tryBoth == true)
			{
				sideID = SIDE_FRONT;
			}
		}
		else
		{
			line = NULL;
		}

		if (line != NULL && line->sidenum[sideID] != 0xffff)
		{
			side = &sides[line->sidenum[sideID]];
		}
		else
		{
			side = NULL;
		}

#undef PROP_TEXTURE
#undef PROP_STR
#undef PROP_INT
#undef PROP_READONLY

	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_Get/SetSectorProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Generic sector property management.
--------------------------------------------------*/
enum
{
	SECTOR_PROP_FLOORHEIGHT,
	SECTOR_PROP_CEILINGHEIGHT,
	SECTOR_PROP_FLOORPIC,
	SECTOR_PROP_CEILINGPIC,
	SECTOR_PROP_LIGHTLEVEL,
	SECTOR_PROP_FLOORLIGHTLEVEL,
	SECTOR_PROP_CEILINGLIGHTLEVEL,
	SECTOR_PROP_FLOORLIGHTABSOLUTE,
	SECTOR_PROP_CEILINGLIGHTABSOLUTE,
	SECTOR_PROP_FLAGS,
	SECTOR_PROP_SPECIALFLAGS,
	SECTOR_PROP_GRAVITY,
	SECTOR_PROP_ACTIVATION,
	SECTOR_PROP_ACTION,
	SECTOR_PROP_ARG0,
	SECTOR_PROP_ARG1,
	SECTOR_PROP_ARG2,
	SECTOR_PROP_ARG3,
	SECTOR_PROP_ARG4,
	SECTOR_PROP_ARG5,
	SECTOR_PROP_ARG6,
	SECTOR_PROP_ARG7,
	SECTOR_PROP_ARG8,
	SECTOR_PROP_ARG9,
	SECTOR_PROP_ARG0STR,
	SECTOR_PROP_ARG1STR,
	SECTOR_PROP__MAX
};

static INT32 NextSector(mtag_t tag, size_t *iterate, INT32 activatorID)
{
	size_t i = *iterate;
	*iterate = *iterate + 1;

	if (tag == 0)
	{
		// 0 grabs the activator.

		if (i != 0)
		{
			// Don't do more than once.
			return -1;
		}

		return activatorID;
	}

	return Tag_Iterate_Sectors(tag, i);
}

bool CallFunc_GetSectorProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;
	Environment *env = &ACSEnv;

	mtag_t tag = 0;
	size_t tagIt = 0;

	INT32 sectorID = 0;
	INT32 activatorID = -1;
	sector_t *sector = NULL;

	INT32 property = SECTOR_PROP__MAX;
	INT32 value = 0;

	tag = argV[0];

	if (info != NULL && info->sector != NULL)
	{
		activatorID = info->sector - sectors;
	}

	if ((sectorID = NextSector(tag, &tagIt, activatorID)) != -1)
	{
		sector = &sectors[ sectorID ];
	}

	property = argV[1];

	if (sector != NULL)
	{

#define PROP_INT(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( sector->y ); \
		break; \
	}

#define PROP_STR(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( ~env->getString( sector->y )->idx ); \
		break; \
	}

#define PROP_FLAT(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( ~env->getString( levelflats[ sector->y ].name )->idx ); \
		break; \
	}

		switch (property)
		{
			PROP_INT(SECTOR_PROP_FLOORHEIGHT, floorheight)
			PROP_INT(SECTOR_PROP_CEILINGHEIGHT, ceilingheight)
			PROP_FLAT(SECTOR_PROP_FLOORPIC, floorpic)
			PROP_FLAT(SECTOR_PROP_CEILINGPIC, ceilingpic)
			PROP_INT(SECTOR_PROP_LIGHTLEVEL, lightlevel)
			PROP_INT(SECTOR_PROP_FLOORLIGHTLEVEL, floorlightlevel)
			PROP_INT(SECTOR_PROP_CEILINGLIGHTLEVEL, ceilinglightlevel)
			PROP_INT(SECTOR_PROP_FLOORLIGHTABSOLUTE, floorlightabsolute)
			PROP_INT(SECTOR_PROP_CEILINGLIGHTABSOLUTE, ceilinglightabsolute)
			PROP_INT(SECTOR_PROP_FLAGS, flags)
			PROP_INT(SECTOR_PROP_SPECIALFLAGS, specialflags)
			PROP_INT(SECTOR_PROP_GRAVITY, gravity)
			PROP_INT(SECTOR_PROP_ACTIVATION, activation)
			PROP_INT(SECTOR_PROP_ACTION, action)
			PROP_INT(SECTOR_PROP_ARG0, args[0])
			PROP_INT(SECTOR_PROP_ARG1, args[1])
			PROP_INT(SECTOR_PROP_ARG2, args[2])
			PROP_INT(SECTOR_PROP_ARG3, args[3])
			PROP_INT(SECTOR_PROP_ARG4, args[4])
			PROP_INT(SECTOR_PROP_ARG5, args[5])
			PROP_INT(SECTOR_PROP_ARG6, args[6])
			PROP_INT(SECTOR_PROP_ARG7, args[7])
			PROP_INT(SECTOR_PROP_ARG8, args[8])
			PROP_INT(SECTOR_PROP_ARG9, args[9])
			PROP_STR(SECTOR_PROP_ARG0STR, stringargs[0])
			PROP_STR(SECTOR_PROP_ARG1STR, stringargs[1])
			default:
			{
				CONS_Alert(CONS_WARNING, "GetSectorProperty type %d out of range (expected 0 - %d).\n", property, SECTOR_PROP__MAX-1);
				break;
			}
		}

#undef PROP_FLAT
#undef PROP_STR
#undef PROP_INT

	}

	thread->dataStk.push(value);
	return false;
}

bool CallFunc_SetSectorProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;
	//Environment *env = &ACSEnv;

	mtag_t tag = 0;
	size_t tagIt = 0;

	INT32 sectorID = 0;
	INT32 activatorID = -1;
	sector_t *sector = NULL;

	INT32 property = SECTOR_PROP__MAX;
	INT32 value = 0;

	tag = argV[0];

	if (info != NULL && info->sector != NULL)
	{
		activatorID = info->sector - sectors;
	}

	if ((sectorID = NextSector(tag, &tagIt, activatorID)) != -1)
	{
		sector = &sectors[ sectorID ];
	}

	property = argV[1];
	value = argV[2];

	while (sector != NULL)
	{

#define PROP_READONLY(x, y) \
	case x: \
	{ \
		CONS_Alert(CONS_WARNING, "SetSectorProperty type '%s' cannot be written to.\n", "y"); \
		break; \
	}

#define PROP_INT(x, y) \
	case x: \
	{ \
		sector->y = static_cast< decltype(sector->y) >(value); \
		break; \
	}

#define PROP_STR(x, y) \
	case x: \
	{ \
		ACSVM::String *str = thread->scopeMap->getString( value ); \
		if (str->len == 0) \
		{ \
			Z_Free(sector->y); \
			sector->y = NULL; \
		} \
		else \
		{ \
			sector->y = static_cast<char *>(Z_Realloc(sector->y, str->len + 1, PU_LEVEL, NULL)); \
			M_Memcpy(sector->y, str->str, str->len + 1); \
			sector->y[str->len] = '\0'; \
		} \
		break; \
	}

#define PROP_FLAT(x, y) \
	case x: \
	{ \
		sector->y = P_AddLevelFlatRuntime( thread->scopeMap->getString( value )->str ); \
		break; \
	}

		switch (property)
		{
			PROP_INT(SECTOR_PROP_FLOORHEIGHT, floorheight)
			PROP_INT(SECTOR_PROP_CEILINGHEIGHT, ceilingheight)
			PROP_FLAT(SECTOR_PROP_FLOORPIC, floorpic)
			PROP_FLAT(SECTOR_PROP_CEILINGPIC, ceilingpic)
			PROP_INT(SECTOR_PROP_LIGHTLEVEL, lightlevel)
			PROP_INT(SECTOR_PROP_FLOORLIGHTLEVEL, floorlightlevel)
			PROP_INT(SECTOR_PROP_CEILINGLIGHTLEVEL, ceilinglightlevel)
			PROP_INT(SECTOR_PROP_FLOORLIGHTABSOLUTE, floorlightabsolute)
			PROP_INT(SECTOR_PROP_CEILINGLIGHTABSOLUTE, ceilinglightabsolute)
			PROP_INT(SECTOR_PROP_FLAGS, flags)
			PROP_INT(SECTOR_PROP_SPECIALFLAGS, specialflags)
			PROP_INT(SECTOR_PROP_GRAVITY, gravity)
			PROP_INT(SECTOR_PROP_ACTIVATION, activation)
			PROP_INT(SECTOR_PROP_ACTION, action)
			PROP_INT(SECTOR_PROP_ARG0, args[0])
			PROP_INT(SECTOR_PROP_ARG1, args[1])
			PROP_INT(SECTOR_PROP_ARG2, args[2])
			PROP_INT(SECTOR_PROP_ARG3, args[3])
			PROP_INT(SECTOR_PROP_ARG4, args[4])
			PROP_INT(SECTOR_PROP_ARG5, args[5])
			PROP_INT(SECTOR_PROP_ARG6, args[6])
			PROP_INT(SECTOR_PROP_ARG7, args[7])
			PROP_INT(SECTOR_PROP_ARG8, args[8])
			PROP_INT(SECTOR_PROP_ARG9, args[9])
			PROP_STR(SECTOR_PROP_ARG0STR, stringargs[0])
			PROP_STR(SECTOR_PROP_ARG1STR, stringargs[1])
			default:
			{
				CONS_Alert(CONS_WARNING, "SetSectorProperty type %d out of range (expected 0 - %d).\n", property, SECTOR_PROP__MAX-1);
				break;
			}
		}

		if ((sectorID = NextSector(tag, &tagIt, activatorID)) != -1)
		{
			sector = &sectors[ sectorID ];
		}
		else
		{
			sector = NULL;
		}

#undef PROP_FLAT
#undef PROP_STR
#undef PROP_INT
#undef PROP_READONLY

	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_Get/SetThingProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Generic thing property management.
--------------------------------------------------*/
enum
{
	THING_PROP_FRAME,
	THING_PROP_SPRITE,
	THING_PROP_SPRITE2,
	THING_PROP_RENDERFLAGS,
	THING_PROP_SPRITEXSCALE,
	THING_PROP_SPRITEYSCALE,
	THING_PROP_SPRITEXOFFSET,
	THING_PROP_SPRITEYOFFSET,
	THING_PROP_SPRITEROLL,
	THING_PROP_RADIUS,
	THING_PROP_HEIGHT,
	THING_PROP_TICS,
	THING_PROP_SKIN,
	THING_PROP_COLOR,
	THING_PROP_HEALTH,
	THING_PROP_MOVEDIR,
	THING_PROP_MOVECOUNT,
	THING_PROP_REACTIONTIME,
	THING_PROP_THRESHOLD,
	THING_PROP_LASTLOOK,
	THING_PROP_FRICTION,
	THING_PROP_MOVEFACTOR,
	THING_PROP_FUSE,
	THING_PROP_WATERTOP,
	THING_PROP_WATERBOTTOM,
	THING_PROP_SCALE,
	THING_PROP_DESTSCALE,
	THING_PROP_SCALESPEED,
	THING_PROP_EXTRAVALUE1,
	THING_PROP_EXTRAVALUE2,
	THING_PROP_CUSVAL,
	THING_PROP_CVMEM,
	THING_PROP_COLORIZED,
	THING_PROP_MIRRORED,
	THING_PROP_SHADOWSCALE,
	THING_PROP_DISPOFFSET,
	THING_PROP_TARGET,
	THING_PROP_TRACER,
	THING_PROP_HNEXT,
	THING_PROP_HPREV,
	THING_PROP__MAX
};

bool CallFunc_GetThingProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;
	Environment *env = &ACSEnv;

	mtag_t tag = 0;
	mobj_t *mobj = NULL;

	INT32 property = SECTOR_PROP__MAX;
	INT32 value = 0;

	tag = argV[0];
	mobj = P_FindMobjFromTID(tag, mobj, info->mo);

	property = argV[1];

	if (mobj != NULL)
	{

#define PROP_INT(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( mobj->y ); \
		break; \
	}

#define PROP_STR(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( ~env->getString( mobj->y )->idx ); \
		break; \
	}

#define PROP_ANGLE(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( AngleFixed( mobj->y ) ); \
		break; \
	}

#define PROP_SPR(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( ~env->getString( sprnames[ mobj->y ] )->idx ); \
		break; \
	}

#define PROP_SPR2(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( ~env->getString( spr2names[ mobj->y ] )->idx ); \
		break; \
	}

#define PROP_SKIN(x, y) \
	case x: \
	{ \
		if (mobj->y != NULL) \
		{ \
			skin_t *skin = static_cast<skin_t *>(mobj->y); \
			value = static_cast<INT32>( ~env->getString( skin->name )->idx ); \
		} \
		break; \
	}

#define PROP_COLOR(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( ~env->getString( skincolors[ mobj->y ].name )->idx ); \
		break; \
	}

#define PROP_MOBJ(x, y) \
	case x: \
	{ \
		if (P_MobjWasRemoved(mobj->y) == false) \
		{ \
			value = static_cast<INT32>( mobj->y->tid ); \
		} \
		break; \
	}

		switch (property)
		{
			PROP_INT(THING_PROP_FRAME, frame)
			PROP_SPR(THING_PROP_SPRITE, sprite)
			PROP_SPR2(THING_PROP_SPRITE2, sprite2)
			PROP_INT(THING_PROP_RENDERFLAGS, renderflags)
			PROP_INT(THING_PROP_SPRITEXSCALE, spritexscale)
			PROP_INT(THING_PROP_SPRITEYSCALE, spriteyscale)
			PROP_INT(THING_PROP_SPRITEXOFFSET, spritexoffset)
			PROP_INT(THING_PROP_SPRITEYOFFSET, spriteyoffset)
			PROP_ANGLE(THING_PROP_SPRITEROLL, spriteroll)
			PROP_INT(THING_PROP_RADIUS, radius)
			PROP_INT(THING_PROP_HEIGHT, height)
			PROP_INT(THING_PROP_TICS, tics)
			PROP_SKIN(THING_PROP_SKIN, skin)
			PROP_COLOR(THING_PROP_COLOR, color)
			PROP_INT(THING_PROP_HEALTH, health)
			PROP_INT(THING_PROP_MOVEDIR, movedir)
			PROP_INT(THING_PROP_MOVECOUNT, movecount)
			PROP_INT(THING_PROP_REACTIONTIME, reactiontime)
			PROP_INT(THING_PROP_THRESHOLD, threshold)
			PROP_INT(THING_PROP_LASTLOOK, lastlook)
			PROP_INT(THING_PROP_FRICTION, friction)
			PROP_INT(THING_PROP_MOVEFACTOR, movefactor)
			PROP_INT(THING_PROP_FUSE, fuse)
			PROP_INT(THING_PROP_WATERTOP, watertop)
			PROP_INT(THING_PROP_WATERBOTTOM, waterbottom)
			PROP_INT(THING_PROP_SCALE, scale)
			PROP_INT(THING_PROP_DESTSCALE, destscale)
			PROP_INT(THING_PROP_SCALESPEED, scalespeed)
			PROP_INT(THING_PROP_EXTRAVALUE1, extravalue1)
			PROP_INT(THING_PROP_EXTRAVALUE2, extravalue2)
			PROP_INT(THING_PROP_CUSVAL, cusval)
			PROP_INT(THING_PROP_CVMEM, cvmem)
			PROP_INT(THING_PROP_COLORIZED, colorized)
			PROP_INT(THING_PROP_MIRRORED, mirrored)
			PROP_INT(THING_PROP_SHADOWSCALE, shadowscale)
			PROP_INT(THING_PROP_DISPOFFSET, dispoffset)
			PROP_MOBJ(THING_PROP_TARGET, target)
			PROP_MOBJ(THING_PROP_TRACER, tracer)
			PROP_MOBJ(THING_PROP_HNEXT, hnext)
			PROP_MOBJ(THING_PROP_HPREV, hprev)
			default:
			{
				CONS_Alert(CONS_WARNING, "GetThingProperty type %d out of range (expected 0 - %d).\n", property, THING_PROP__MAX-1);
				break;
			}
		}

#undef PROP_MOBJ
#undef PROP_COLOR
#undef PROP_SKIN
#undef PROP_SPR2
#undef PROP_SPR
#undef PROP_ANGLE
#undef PROP_STR
#undef PROP_INT

	}

	thread->dataStk.push(value);
	return false;
}

bool CallFunc_SetThingProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;
	//Environment *env = &ACSEnv;

	mtag_t tag = 0;
	mobj_t *mobj = NULL;

	INT32 property = SECTOR_PROP__MAX;
	INT32 value = 0;

	tag = argV[0];
	mobj = P_FindMobjFromTID(tag, mobj, info->mo);

	property = argV[1];
	value = argV[2];

	while (mobj != NULL)
	{

#define PROP_READONLY(x, y) \
	case x: \
	{ \
		CONS_Alert(CONS_WARNING, "SetThingProperty type '%s' cannot be written to.\n", "y"); \
		break; \
	}

#define PROP_INT(x, y) \
	case x: \
	{ \
		mobj->y = static_cast< decltype(mobj->y) >(value); \
		break; \
	}

#define PROP_STR(x, y) \
	case x: \
	{ \
		ACSVM::String *str = thread->scopeMap->getString( value ); \
		if (str->len == 0) \
		{ \
			Z_Free(mobj->y); \
			mobj->y = NULL; \
		} \
		else \
		{ \
			mobj->y = static_cast<char *>(Z_Realloc(mobj->y, str->len + 1, PU_LEVEL, NULL)); \
			M_Memcpy(mobj->y, str->str, str->len + 1); \
			mobj->y[str->len] = '\0'; \
		} \
		break; \
	}

#define PROP_ANGLE(x, y) \
	case x: \
	{ \
		mobj->y = static_cast<angle_t>( FixedAngle(value) ); \
		break; \
	}

#define PROP_SPR(x, y) \
	case x: \
	{ \
		spritenum_t newSprite = mobj->y; \
		bool success = ACS_GetSpriteFromString(thread->scopeMap->getString( value )->str, &newSprite); \
		if (success == true) \
		{ \
			mobj->y = newSprite; \
		} \
		break; \
	}

#define PROP_SPR2(x, y) \
	case x: \
	{ \
		playersprite_t newSprite2 = static_cast<playersprite_t>(mobj->y); \
		bool success = ACS_GetSprite2FromString(thread->scopeMap->getString( value )->str, &newSprite2); \
		if (success == true) \
		{ \
			mobj->y = static_cast< decltype(mobj->y) >(newSprite2); \
		} \
		break; \
	}

#define PROP_SKIN(x, y) \
	case x: \
	{ \
		INT32 newSkin = (mobj->skin != NULL) ? (static_cast<skin_t *>(mobj->skin))->skinnum : -1; \
		bool success = ACS_GetSkinFromString(thread->scopeMap->getString( value )->str, &newSkin); \
		if (success == true) \
		{ \
			mobj->y = (newSkin >= 0 && newSkin < numskins) ? &skins[ newSkin ] : NULL; \
		} \
		break; \
	}

#define PROP_COLOR(x, y) \
	case x: \
	{ \
		skincolornum_t newColor = static_cast<skincolornum_t>(mobj->y); \
		bool success = ACS_GetColorFromString(thread->scopeMap->getString( value )->str, &newColor); \
		if (success == true) \
		{ \
			mobj->y = static_cast< decltype(mobj->y) >(newColor); \
		} \
		break; \
	}

#define PROP_MOBJ(x, y) \
	case x: \
	{ \
		mobj_t *newTarget = P_FindMobjFromTID(value, NULL, NULL); \
		P_SetTarget(&mobj->y, newTarget); \
		break; \
	}

#define PROP_SCALE(x, y) \
	case x: \
	{ \
		P_SetScale(mobj, value, false); \
		break; \
	}

		switch (property)
		{
			PROP_INT(THING_PROP_FRAME, frame)
			PROP_SPR(THING_PROP_SPRITE, sprite)
			PROP_SPR2(THING_PROP_SPRITE2, sprite2)
			PROP_INT(THING_PROP_RENDERFLAGS, renderflags)
			PROP_INT(THING_PROP_SPRITEXSCALE, spritexscale)
			PROP_INT(THING_PROP_SPRITEYSCALE, spriteyscale)
			PROP_INT(THING_PROP_SPRITEXOFFSET, spritexoffset)
			PROP_INT(THING_PROP_SPRITEYOFFSET, spriteyoffset)
			PROP_ANGLE(THING_PROP_SPRITEROLL, spriteroll)
			PROP_READONLY(THING_PROP_RADIUS, radius)
			PROP_READONLY(THING_PROP_HEIGHT, height)
			PROP_INT(THING_PROP_TICS, tics)
			PROP_SKIN(THING_PROP_SKIN, skin)
			PROP_COLOR(THING_PROP_COLOR, color)
			PROP_INT(THING_PROP_HEALTH, health)
			PROP_INT(THING_PROP_MOVEDIR, movedir)
			PROP_INT(THING_PROP_MOVECOUNT, movecount)
			PROP_INT(THING_PROP_REACTIONTIME, reactiontime)
			PROP_INT(THING_PROP_THRESHOLD, threshold)
			PROP_INT(THING_PROP_LASTLOOK, lastlook)
			PROP_INT(THING_PROP_FRICTION, friction)
			PROP_INT(THING_PROP_MOVEFACTOR, movefactor)
			PROP_INT(THING_PROP_FUSE, fuse)
			PROP_INT(THING_PROP_WATERTOP, watertop)
			PROP_INT(THING_PROP_WATERBOTTOM, waterbottom)
			PROP_SCALE(THING_PROP_SCALE, scale)
			PROP_INT(THING_PROP_DESTSCALE, destscale)
			PROP_INT(THING_PROP_SCALESPEED, scalespeed)
			PROP_INT(THING_PROP_EXTRAVALUE1, extravalue1)
			PROP_INT(THING_PROP_EXTRAVALUE2, extravalue2)
			PROP_INT(THING_PROP_CUSVAL, cusval)
			PROP_INT(THING_PROP_CVMEM, cvmem)
			PROP_INT(THING_PROP_COLORIZED, colorized)
			PROP_INT(THING_PROP_MIRRORED, mirrored)
			PROP_INT(THING_PROP_SHADOWSCALE, shadowscale)
			PROP_INT(THING_PROP_DISPOFFSET, dispoffset)
			PROP_MOBJ(THING_PROP_TARGET, target)
			PROP_MOBJ(THING_PROP_TRACER, tracer)
			PROP_MOBJ(THING_PROP_HNEXT, hnext)
			PROP_MOBJ(THING_PROP_HPREV, hprev)
			default:
			{
				CONS_Alert(CONS_WARNING, "SetThingProperty type %d out of range (expected 0 - %d).\n", property, THING_PROP__MAX-1);
				break;
			}
		}

		mobj = P_FindMobjFromTID(tag, mobj, info->mo);

#undef PROP_SCALE
#undef PROP_MOBJ
#undef PROP_COLOR
#undef PROP_SKIN
#undef PROP_SPR2
#undef PROP_SPR
#undef PROP_ANGLE
#undef PROP_STR
#undef PROP_INT
#undef PROP_READONLY

	}

	NO_RETURN(thread);

	return false;
}

enum
{
	POWER_INVULNERABILITY,
	POWER_SNEAKERS,
	POWER_FLASHING,
	POWER_SHIELD,
	POWER_TAILSFLY,
	POWER_UNDERWATER,
	POWER_SPACETIME,
	POWER_GRAVITYBOOTS,
	POWER_EMERALDS,
	POWER_NIGHTS_SUPERLOOP,
	POWER_NIGHTS_HELPER,
	POWER_NIGHTS_LINKFREEZE,
	POWER_AUTOMATICRING,
	POWER_BOUNCERING,
	POWER_SCATTERRING,
	POWER_GRENADERING,
	POWER_EXPLOSIONRING,
	POWER_RAILRING,
	POWER__MAX
};

/*--------------------------------------------------
	bool CallFunc_CheckPowerUp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Checks the amount of the activator's given power-up.
--------------------------------------------------*/
bool CallFunc_CheckPowerUp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	INT32 value = 0;

	auto info = &static_cast<Thread *>(thread)->info;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		INT32 property = argV[0];

		player_t *player = info->mo->player;

#define POWER(x, y) \
	case x: \
	{ \
		value = player->powers[y]; \
		break; \
	}

#define WEAPON(x, y) \
	case x: \
	{ \
		value = (player->ringweapons & y) != 0; \
		break; \
	}

		switch (property)
		{
			POWER(POWER_INVULNERABILITY, pw_invulnerability)
			POWER(POWER_SNEAKERS, pw_sneakers)
			POWER(POWER_FLASHING, pw_flashing)
			POWER(POWER_SHIELD, pw_shield)
			POWER(POWER_TAILSFLY, pw_tailsfly)
			POWER(POWER_UNDERWATER, pw_underwater)
			POWER(POWER_SPACETIME, pw_spacetime)
			POWER(POWER_GRAVITYBOOTS, pw_gravityboots)
			POWER(POWER_EMERALDS, pw_emeralds)
			POWER(POWER_NIGHTS_SUPERLOOP, pw_nights_superloop)
			POWER(POWER_NIGHTS_HELPER, pw_nights_helper)
			POWER(POWER_NIGHTS_LINKFREEZE, pw_nights_linkfreeze)
			WEAPON(POWER_AUTOMATICRING, WEP_AUTO)
			WEAPON(POWER_BOUNCERING, WEP_BOUNCE)
			WEAPON(POWER_SCATTERRING, WEP_SCATTER)
			WEAPON(POWER_GRENADERING, WEP_GRENADE)
			WEAPON(POWER_EXPLOSIONRING, WEP_EXPLODE)
			WEAPON(POWER_RAILRING, WEP_RAIL)
			default:
			{
				CONS_Alert(CONS_WARNING, "CheckPowerUp type %d out of range (expected 0 - %d).\n", property, POWER__MAX-1);
				break;
			}
		}

#undef POWER
#undef WEAPON
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	static void ACS_SetPowerUp(player_t *player, INT32 property, const ACSVM::Word *argV, ACSVM::Word argC)

		Helper function for CallFunc_GivePowerUp.
--------------------------------------------------*/
static void ACS_SetPowerUp(player_t *player, INT32 property, const ACSVM::Word *argV, ACSVM::Word argC)
{
#define POWER(x, y, z) \
	case x: \
	{ \
		if (argC >= 2) \
			player->powers[y] = argV[1]; \
		else \
			player->powers[y] = z; \
		break; \
	}

#define WEAPON(x, y) \
	case x: \
	{ \
		if (argC >= 2 && (argV[1] > 0)) \
			player->ringweapons |= y; \
		else \
			player->ringweapons &= ~y; \
		break; \
	}

	switch (property)
	{
		POWER(POWER_INVULNERABILITY, pw_invulnerability, invulntics)
		POWER(POWER_SNEAKERS, pw_sneakers, sneakertics)
		POWER(POWER_FLASHING, pw_flashing, flashingtics)
		POWER(POWER_SHIELD, pw_shield, 0)
		POWER(POWER_TAILSFLY, pw_tailsfly, tailsflytics)
		POWER(POWER_UNDERWATER, pw_underwater, underwatertics)
		POWER(POWER_SPACETIME, pw_spacetime, spacetimetics)
		POWER(POWER_GRAVITYBOOTS, pw_gravityboots, 20*TICRATE)
		POWER(POWER_EMERALDS, pw_emeralds, 0)
		POWER(POWER_NIGHTS_SUPERLOOP, pw_nights_superloop, 0)
		POWER(POWER_NIGHTS_HELPER, pw_nights_helper, 0)
		POWER(POWER_NIGHTS_LINKFREEZE, pw_nights_linkfreeze, 0)
		WEAPON(POWER_AUTOMATICRING, WEP_AUTO)
		WEAPON(POWER_BOUNCERING, WEP_BOUNCE)
		WEAPON(POWER_SCATTERRING, WEP_SCATTER)
		WEAPON(POWER_GRENADERING, WEP_GRENADE)
		WEAPON(POWER_EXPLOSIONRING, WEP_EXPLODE)
		WEAPON(POWER_RAILRING, WEP_RAIL)
		default:
		{
			CONS_Alert(CONS_WARNING, "GivePowerUp type %d out of range (expected 0 - %d).\n", property, POWER__MAX-1);
			break;
		}
	}

	// Give the player a shield
	if (property == POWER_SHIELD && player->powers[pw_shield] != SH_NONE)
	{
		P_SpawnShieldOrb(player);
	}

#undef POWER
#undef AMMO
}

/*--------------------------------------------------
	bool CallFunc_GivePowerUp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Awards a power-up to the activator.
		Like GiveInventory, if this is called with no activator, this awards the power-up to all players.
--------------------------------------------------*/
bool CallFunc_GivePowerUp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	INT32 property = argV[0];

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		ACS_SetPowerUp(info->mo->player, property, argV, argC);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				ACS_SetPowerUp(&players[i], property, argV, argC);
		}
	}

	NO_RETURN(thread);

	return false;
}

enum
{
	WEAPON_AUTO,
	WEAPON_BOUNCE,
	WEAPON_SCATTER,
	WEAPON_GRENADE,
	WEAPON_EXPLODE,
	WEAPON_RAIL,
	WEAPON__MAX
};

/*--------------------------------------------------
	bool CallFunc_CheckAmmo(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Checks the ammo of the activator's weapon.
--------------------------------------------------*/
bool CallFunc_CheckAmmo(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	INT32 value = 0;

	auto info = &static_cast<Thread *>(thread)->info;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		INT32 weapon = argV[0];

		player_t *player = info->mo->player;

#define AMMO(x, y) \
	case x: \
	{ \
		value = player->powers[y]; \
		break; \
	}

		switch (weapon)
		{
			AMMO(WEAPON_AUTO, pw_automaticring)
			AMMO(WEAPON_BOUNCE, pw_bouncering)
			AMMO(WEAPON_SCATTER, pw_scatterring)
			AMMO(WEAPON_GRENADE, pw_grenadering)
			AMMO(WEAPON_EXPLODE, pw_explosionring)
			AMMO(WEAPON_RAIL, pw_railring)
			default:
			{
				CONS_Alert(CONS_WARNING, "GiveAmmo weapon %d out of range (expected 0 - %d).\n", weapon, WEAPON__MAX-1);
				break;
			}
		}

#undef AMMO
	}

	thread->dataStk.push(value);

	return false;
}

/*--------------------------------------------------
	static void ACS_GiveAmmo(player_t *player, INT32 weapon, INT32 value)

		Helper function for CallFunc_GiveAmmo/CallFunc_TakeAmmo.
--------------------------------------------------*/
static bool ACS_GiveAmmo(player_t *player, INT32 weapon, INT32 value)
{
#define AMMO(x, y) \
	case x: \
	{ \
		player->powers[y] += value; \
		return true; \
	}

	switch (weapon)
	{
		AMMO(WEAPON_AUTO, pw_automaticring)
		AMMO(WEAPON_BOUNCE, pw_bouncering)
		AMMO(WEAPON_SCATTER, pw_scatterring)
		AMMO(WEAPON_GRENADE, pw_grenadering)
		AMMO(WEAPON_EXPLODE, pw_explosionring)
		AMMO(WEAPON_RAIL, pw_railring)
	}

#undef AMMO

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GiveAmmo(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Gives ammo to the activator.
		Like GiveInventory, if this is called with no activator, this awards ammo to all players.
--------------------------------------------------*/
bool CallFunc_GiveAmmo(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	INT32 weapon = argV[0];
	INT32 value = argC >= 2 ? (argV[1]) : 0;

	bool fail = false;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		fail = !ACS_GiveAmmo(info->mo->player, weapon, value);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				if (!ACS_GiveAmmo(&players[i], weapon, value))
				{
					fail = true;
					break;
				}
			}
		}
	}

	if (fail)
	{
		CONS_Alert(CONS_WARNING, "GiveAmmo weapon %d out of range (expected 0 - %d).\n", weapon, WEAPON__MAX-1);
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_TakeAmmo(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Takes ammo from the activator.
		Like TakeInventory, if this is called with no activator, this takes ammo from all players.
--------------------------------------------------*/
bool CallFunc_TakeAmmo(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	INT32 weapon = argV[0];
	INT32 value = argC >= 2 ? (argV[1]) : 0;

	bool fail = false;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		fail = !ACS_GiveAmmo(info->mo->player, weapon, -value);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				if (!ACS_GiveAmmo(&players[i], weapon, -value))
				{
					fail = true;
					break;
				}
			}
		}
	}

	if (fail)
	{
		CONS_Alert(CONS_WARNING, "TakeAmmo weapon %d out of range (expected 0 - %d).\n", weapon, WEAPON__MAX-1);
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_DoSuperTransformation(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Turns the activator super.
--------------------------------------------------*/
bool CallFunc_DoSuperTransformation(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	bool giverings = argC >= 2 ? (argV[1] != 0) : false;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		P_DoSuperTransformation(info->mo->player, giverings);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				P_DoSuperTransformation(&players[i], giverings);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_DoSuperTransformation(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Detransforms the activator if super.
--------------------------------------------------*/
bool CallFunc_DoSuperDetransformation(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		P_DoSuperDetransformation(info->mo->player);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				P_DoSuperDetransformation(&players[i]);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GiveRings(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Awards a specific amount of rings to the activator.
--------------------------------------------------*/
bool CallFunc_GiveRings(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	INT32 numRings = argV[0];

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		P_GivePlayerRings(info->mo->player, numRings);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				P_GivePlayerRings(&players[i], numRings);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GiveSpheres(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Awards a specific amount of spheres to the activator.
--------------------------------------------------*/
bool CallFunc_GiveSpheres(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	INT32 numSpheres = argV[0];

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		P_GivePlayerSpheres(info->mo->player, numSpheres);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				P_GivePlayerSpheres(&players[i], numSpheres);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GiveLives(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Awards a specific amount of lives to the activator.
--------------------------------------------------*/
bool CallFunc_GiveLives(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	INT32 numLives = argV[0];

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		P_GivePlayerLives(info->mo->player, numLives);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				P_GivePlayerLives(&players[i], numLives);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GiveScore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Awards score to the activator.
--------------------------------------------------*/
bool CallFunc_GiveScore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	auto info = &static_cast<Thread *>(thread)->info;

	INT32 score = argV[0];

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		P_AddPlayerScore(info->mo->player, score);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				P_AddPlayerScore(&players[i], score);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_DropFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Makes the activator drop their flag (or all players if there was no activator.)
--------------------------------------------------*/
bool CallFunc_DropFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	(void)argV;

	auto info = &static_cast<Thread *>(thread)->info;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		P_PlayerFlagBurst(info->mo->player, false);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				P_PlayerFlagBurst(&players[i], false);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_TossFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Makes the activator toss their flag (or all players if there was no activator.)
--------------------------------------------------*/
bool CallFunc_TossFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	(void)argV;

	auto info = &static_cast<Thread *>(thread)->info;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		P_PlayerFlagBurst(info->mo->player, true);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				P_PlayerFlagBurst(&players[i], true);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_DropEmeralds(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Makes the activator drop their emeralds (or all players if there was no activator.)
--------------------------------------------------*/
bool CallFunc_DropEmeralds(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	(void)argV;

	auto info = &static_cast<Thread *>(thread)->info;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		P_PlayerEmeraldBurst(info->mo->player, false);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				P_PlayerEmeraldBurst(&players[i], false);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_TossEmeralds(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Makes the activator toss their emeralds (or all players if there was no activator.)
--------------------------------------------------*/
bool CallFunc_TossEmeralds(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	(void)argV;

	auto info = &static_cast<Thread *>(thread)->info;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		P_PlayerEmeraldBurst(info->mo->player, true);
	}
	else
	{
		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				P_PlayerEmeraldBurst(&players[i], true);
		}
	}

	NO_RETURN(thread);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerHoldingFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Checks if the given player is holding a flag.
--------------------------------------------------*/
bool CallFunc_PlayerHoldingFlag(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	INT32 playernum = argV[0];
	INT32 gotflag = argV[1];

	bool isHoldingFlag = false;

	if (playernum < 0 || playernum >= MAXPLAYERS)
	{
		CONS_Alert(CONS_WARNING, "PlayerHoldingFlag player %d out of range (expected 0 - %d).\n", playernum, MAXPLAYERS-1);
	}
	else
	{
		if (players[playernum].gotflag & gotflag)
			isHoldingFlag = true;
	}

	thread->dataStk.push(isHoldingFlag);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerIsIt(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Checks if the given player is it.
--------------------------------------------------*/
bool CallFunc_PlayerIsIt(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	INT32 playernum = argV[0];

	bool isIt = false;

	if (playernum < 0 || playernum >= MAXPLAYERS)
	{
		CONS_Alert(CONS_WARNING, "PlayerIsIt player %d out of range (expected 0 - %d).\n", playernum, MAXPLAYERS-1);
	}
	else
	{
		if (players[playernum].pflags & PF_TAGIT)
			isIt = true;
	}

	thread->dataStk.push(isIt);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerFinished(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Checks if the given player finished the level.
--------------------------------------------------*/
bool CallFunc_PlayerFinished(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	INT32 playernum = argV[0];

	bool finished = false;

	if (playernum < 0 || playernum >= MAXPLAYERS)
	{
		CONS_Alert(CONS_WARNING, "PlayerFinished player %d out of range (expected 0 - %d).\n", playernum, MAXPLAYERS-1);
	}
	else
	{
		if (players[playernum].pflags & PF_FINISHED)
			finished = true;
	}

	thread->dataStk.push(finished);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_Sin(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the sine of a fixed-point number.
--------------------------------------------------*/
bool CallFunc_Sin(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	thread->dataStk.push(ACS_AngleToFixed(FINESINE((ACS_FixedToAngle(argV[0])>>ANGLETOFINESHIFT) & FINEMASK)));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_Cos(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the cosine of a fixed-point number.
--------------------------------------------------*/
bool CallFunc_Cos(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	thread->dataStk.push(ACS_AngleToFixed(FINECOSINE((ACS_FixedToAngle(argV[0])>>ANGLETOFINESHIFT) & FINEMASK)));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_Tan(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the tangent of a fixed-point number.
--------------------------------------------------*/
bool CallFunc_Tan(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	thread->dataStk.push(ACS_AngleToFixed(FINETANGENT(((ACS_FixedToAngle(argV[0])+ANGLE_90)>>ANGLETOFINESHIFT) & 4095)));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_Asin(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns arcsin(x), where x is a fixed-point number.
--------------------------------------------------*/
bool CallFunc_Arcsin(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	thread->dataStk.push(ACS_AngleToFixed(-FixedAcos(argV[0]) + ANGLE_90));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_Acos(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns arccos(x), where x is a fixed-point number.
--------------------------------------------------*/
bool CallFunc_Arccos(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	thread->dataStk.push(ACS_AngleToFixed(FixedAcos(argV[0])));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_Hypot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns hypot(x, y), where x and y are fixed-point numbers.
--------------------------------------------------*/
bool CallFunc_Hypot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	thread->dataStk.push(R_PointToDist2(0, 0, argV[0], argV[1]));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_Sqrt(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the square root of a fixed-point number.
--------------------------------------------------*/
bool CallFunc_Sqrt(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	int x = argV[0];

	fixed_t result = 0;

	if (x < 0)
	{
		CONS_Alert(CONS_WARNING, "Sqrt: square root domain error\n");
	}
	else
	{
		result = FixedSqrt(x);
	}

	thread->dataStk.push(result);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_Floor(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns floor(x), where x is a fixed-point number.
--------------------------------------------------*/
bool CallFunc_Floor(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	thread->dataStk.push(FixedFloor(argV[0]));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_Ceil(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns ceil(x), where x is a fixed-point number.
--------------------------------------------------*/
bool CallFunc_Ceil(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	thread->dataStk.push(FixedCeil(argV[0]));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_Round(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns round(x), where x is a fixed-point number.
--------------------------------------------------*/
bool CallFunc_Round(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	thread->dataStk.push(FixedRound(argV[0]));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_InvAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the inverse of an angle.
--------------------------------------------------*/
bool CallFunc_InvAngle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;
	thread->dataStk.push(ACS_AngleToFixed(InvAngle(ACS_FixedToAngle(argV[0]))));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_OppositeColor(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the opposite of a color.
--------------------------------------------------*/
bool CallFunc_OppositeColor(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	Environment *env = &ACSEnv;

	ACSVM::String *str = thread->scopeMap->getString(argV[0]);

	skincolornum_t invColor = SKINCOLOR_NONE;

	if (str->len > 0)
	{
		skincolornum_t color = SKINCOLOR_NONE;

		const char *colorName = str->str;

		bool success = ACS_GetColorFromString(colorName, &color);

		if (success == false)
		{
			// Exit early.
			CONS_Alert(CONS_WARNING,
				"Couldn't find color \"%s\" for OppositeColor.\n",
				colorName
			);

			NO_RETURN(thread);

			return false;
		}

		invColor = static_cast<skincolornum_t>(skincolors[color].invcolor);
	}
	else
	{
		CONS_Alert(CONS_WARNING, "OppositeColor color was not provided.\n");

		NO_RETURN(thread);

		return false;
	}

	thread->dataStk.push(~env->getString( skincolors[invColor].name )->idx);
	return false;
}
