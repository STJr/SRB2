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
#include "../w_wad.h"
#include "../m_misc.h"
#include "../m_random.h"
#include "../g_game.h"
#include "../d_player.h"
#include "../r_defs.h"
#include "../r_state.h"
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
		if (fastncmp(word, sprnames[i], 4))
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
bool ACS_ThingTypeFilter(mobj_t *mo)
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
				"Couldn't find object type \"%s\" for ThingCount.\n",
				className
			);

			return false;
		}
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

	thread->dataStk.push(0);

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

			return false;
		}
	}

	vol = argV[1];

	S_StartSoundAtVolume(NULL, sfxId, vol);
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

	return false;
}

/*--------------------------------------------------
	bool CallFunc_ChangeSky(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Changes the map's sky texture.
--------------------------------------------------*/
bool CallFunc_ChangeSky(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)thread;
	(void)argC;

	P_SetupLevelSky(argV[0], argV[1]);

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
				"Couldn't find sfx named \"%s\" for AmbientSound.\n",
				sfxName
			);

			return false;
		}
	}

	vol = argV[2];

	while ((mobj = P_FindMobjFromTID(tag, mobj, info->mo)) != nullptr)
	{
		S_StartSoundAtVolume(mobj, sfxId, vol);
	}

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
bool ACS_EnemyFilter(mobj_t *mo)
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
bool ACS_PushableFilter(mobj_t *mo)
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
	bool CallFunc_HaveUnlockable(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if an unlockable has been gotten.
--------------------------------------------------*/
bool CallFunc_HaveUnlockable(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	UINT32 id = 0;
	bool unlocked = false;

	(void)argC;

	id = argV[0];

	if (id >= MAXUNLOCKABLES)
	{
		CONS_Printf("Bad unlockable ID %d\n", id);
	}
	else
	{
		unlocked = M_CheckNetUnlockByID(id);
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

	thread->dataStk.push(0);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's bot status.
--------------------------------------------------*/
bool CallFunc_PlayerBot(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{

		thread->dataStk.push(info->mo->player->bot);
		return false;
	}

	thread->dataStk.push(false);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerExiting(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's exiting status.
--------------------------------------------------*/
bool CallFunc_PlayerExiting(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		thread->dataStk.push((info->mo->player->exiting != 0));
		return false;
	}

	thread->dataStk.push(false);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Dyes the activating object.
--------------------------------------------------*/
bool CallFunc_SetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false))
	{
		var1 = 0;
		var2 = argV[0];
		A_Dye(info->mo);
	}

	thread->dataStk.push(0);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating object's current dye.
--------------------------------------------------*/
bool CallFunc_GetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	Environment *env = &ACSEnv;
	auto info = &static_cast<Thread *>(thread)->info;
	UINT16 dye = SKINCOLOR_NONE;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false))
	{
		dye = (info->mo->player != NULL) ? info->mo->player->powers[pw_dye] : info->mo->color;
	}

	thread->dataStk.push(~env->getString( skincolors[dye].name )->idx);
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

	(void)thread;
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

	thread->dataStk.push(0);

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

	INT16 nextmap = 0;

	(void)argC;

	map = thread->scopeMap;

	str = map->getString(argV[0]);

	levelName = str->str;
	levelLen = str->len;

	if (!levelLen || !levelName)
	{
		CONS_Alert(CONS_WARNING, "MapWarp level name was not provided.\n");
	}

	if (levelName[0] == 'M' && levelName[1] == 'A' && levelName[2] == 'P' && levelName[5] == '\0')
	{
		nextmap = (INT16)M_MapNumber(levelName[3], levelName[4]);
	}

	if (nextmap == 0)
	{
		CONS_Alert(CONS_WARNING, "MapWarp level %s is not valid or loaded.\n", levelName);
		return false;
	}

	nextmapoverride = nextmap;

	if (argV[1] == 0)
		skipstats = 1;

	if (server)
		SendNetXCmd(XD_EXITLEVEL, NULL, 0);

	thread->dataStk.push(0);

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

	UINT16 skincolor = SKINCOLOR_NONE;

	SINT8 bottype = BOT_MPAI;

	// Get name
	skinStr = map->getString(argV[0]);
	if (skinStr->len != 0)
		skinname = skinStr->str;

	// Get skincolor
	if (argC >= 2)
		skincolor = std::clamp(static_cast<int>(argV[1]), (int)SKINCOLOR_NONE, (int)(MAXSKINCOLORS - 1));

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
	bool CallFunc_ExitLevel(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Exits the level.
--------------------------------------------------*/
bool CallFunc_ExitLevel(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)thread;
	(void)argV;
	(void)argC;

	if (argC >= 1)
	{
		skipstats = (argV[0] == 0);
	}

	if (server)
		SendNetXCmd(XD_EXITLEVEL, NULL, 0);

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
		return false;
	}

	S_StopMusic();
	S_ChangeMusicInternal(map->getString(argV[0])->str, true);

	thread->dataStk.push(0);

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
		return false;
	}

	S_StopMusic();

	thread->dataStk.push(0);

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

	thread->dataStk.push(0);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_MusicDim(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Fade level music into or out of silence.
--------------------------------------------------*/
bool CallFunc_MusicDim(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)thread;
	(void)argC;

	// 0: int fade time (ms) - time to fade between full volume and silence
	UINT32 fade = argV[0];

	S_FadeOutStopMusic(fade);

	thread->dataStk.push(0);

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
	(void)thread;
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
	(void)thread;
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

	thread->dataStk.push(0);

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
	(void)thread;
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
	(void)thread;
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

	thread->dataStk.push(0);

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
	(void)thread;
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
	(void)thread;
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

	thread->dataStk.push(0);

	return false;
}

/*--------------------------------------------------
	bool CallFunc_Get/SetThingProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Generic thing property management.
--------------------------------------------------*/
enum
{
	THING_PROP_X,
	THING_PROP_Y,
	THING_PROP_Z,
	THING_PROP_TYPE,
	THING_PROP_ANGLE,
	THING_PROP_PITCH,
	THING_PROP_ROLL,
	THING_PROP_SPRITEROLL,
	THING_PROP_FRAME,
	THING_PROP_SPRITE,
	THING_PROP_SPRITE2,
	THING_PROP_RENDERFLAGS,
	THING_PROP_SPRITEXSCALE,
	THING_PROP_SPRITEYSCALE,
	THING_PROP_SPRITEXOFFSET,
	THING_PROP_SPRITEYOFFSET,
	THING_PROP_FLOORZ,
	THING_PROP_CEILINGZ,
	THING_PROP_RADIUS,
	THING_PROP_HEIGHT,
	THING_PROP_MOMX,
	THING_PROP_MOMY,
	THING_PROP_MOMZ,
	THING_PROP_TICS,
	THING_PROP_STATE,
	THING_PROP_FLAGS,
	THING_PROP_FLAGS2,
	THING_PROP_EFLAGS,
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
	(void)thread;
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

#define PROP_TYPE(x, y) \
	case x: \
	{ \
		if (mobj->y >= MT_FIRSTFREESLOT) \
		{ \
			std::string	prefix = "MT_"; \
			std::string	full = prefix + FREE_MOBJS[mobj->y - MT_FIRSTFREESLOT]; \
			value = static_cast<INT32>( ~env->getString( full.c_str() )->idx ); \
		} \
		else \
		{ \
			value = static_cast<INT32>( ~env->getString( MOBJTYPE_LIST[ mobj->y ] )->idx ); \
		} \
		break; \
	}

#define PROP_SPR(x, y) \
	case x: \
	{ \
		char crunched[5] = {0}; \
		strncpy(crunched, sprnames[ mobj->y ], 4); \
		value = static_cast<INT32>( ~env->getString( crunched )->idx ); \
		break; \
	}

#define PROP_SPR2(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( ~env->getString( spr2names[ mobj->y ] )->idx ); \
		break; \
	}

#define PROP_STATE(x, y) \
	case x: \
	{ \
		statenum_t stateID = static_cast<statenum_t>(mobj->y - states); \
		if (stateID >= S_FIRSTFREESLOT) \
		{ \
			std::string	prefix = "S_"; \
			std::string	full = prefix + FREE_STATES[stateID - S_FIRSTFREESLOT]; \
			value = static_cast<INT32>( ~env->getString( full.c_str() )->idx ); \
		} \
		else \
		{ \
			value = static_cast<INT32>( ~env->getString( STATE_LIST[ stateID ] )->idx ); \
		} \
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
			PROP_INT(THING_PROP_X, x)
			PROP_INT(THING_PROP_Y, y)
			PROP_INT(THING_PROP_Z, z)
			PROP_TYPE(THING_PROP_TYPE, type)
			PROP_ANGLE(THING_PROP_ANGLE, angle)
			PROP_ANGLE(THING_PROP_PITCH, pitch)
			PROP_ANGLE(THING_PROP_ROLL, roll)
			PROP_ANGLE(THING_PROP_SPRITEROLL, spriteroll)
			PROP_INT(THING_PROP_FRAME, frame)
			PROP_SPR(THING_PROP_SPRITE, sprite)
			PROP_SPR2(THING_PROP_SPRITE2, sprite2)
			PROP_INT(THING_PROP_RENDERFLAGS, renderflags)
			PROP_INT(THING_PROP_SPRITEXSCALE, spritexscale)
			PROP_INT(THING_PROP_SPRITEYSCALE, spriteyscale)
			PROP_INT(THING_PROP_SPRITEXOFFSET, spritexoffset)
			PROP_INT(THING_PROP_SPRITEYOFFSET, spriteyoffset)
			PROP_INT(THING_PROP_FLOORZ, floorz)
			PROP_INT(THING_PROP_CEILINGZ, ceilingz)
			PROP_INT(THING_PROP_RADIUS, radius)
			PROP_INT(THING_PROP_HEIGHT, height)
			PROP_INT(THING_PROP_MOMX, momx)
			PROP_INT(THING_PROP_MOMY, momy)
			PROP_INT(THING_PROP_MOMZ, momz)
			PROP_INT(THING_PROP_TICS, tics)
			PROP_STATE(THING_PROP_STATE, state)
			PROP_INT(THING_PROP_FLAGS, flags)
			PROP_INT(THING_PROP_FLAGS2, flags2)
			PROP_INT(THING_PROP_EFLAGS, eflags)
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
#undef PROP_STATE
#undef PROP_SPR2
#undef PROP_SPR
#undef PROP_TYPE
#undef PROP_ANGLE
#undef PROP_STR
#undef PROP_INT

	}

	thread->dataStk.push(value);
	return false;
}

bool CallFunc_SetThingProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)thread;
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

#define PROP_TYPE(x, y) \
	case x: \
	{ \
		if (mobj->player == NULL) \
		{ \
			mobjtype_t newType = mobj->y; \
			bool success = ACS_GetMobjTypeFromString(thread->scopeMap->getString( value )->str, &newType); \
			if (success == true) \
			{ \
				mobj->y = newType; \
				mobj->info = &mobjinfo[newType]; \
				P_SetScale(mobj, mobj->scale, false); \
			} \
		} \
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

#define PROP_STATE(x, y) \
	case x: \
	{ \
		statenum_t newState = static_cast<statenum_t>(mobj->y - states); \
		bool success = ACS_GetStateFromString(thread->scopeMap->getString( value )->str, &newState); \
		if (success == true) \
		{ \
			P_SetMobjState(mobj, newState); \
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

#define PROP_FLAGS(x, y) \
	case x: \
	{ \
		if ((value & (MF_NOBLOCKMAP|MF_NOSECTOR)) != (mobj->y & (MF_NOBLOCKMAP|MF_NOSECTOR))) \
		{ \
			P_UnsetThingPosition(mobj); \
			mobj->y = value; \
			if ((value & MF_NOSECTOR) && sector_list) \
			{ \
				P_DelSeclist(sector_list); \
				sector_list = NULL; \
			} \
			mobj->snext = NULL, mobj->sprev = NULL; \
			P_SetThingPosition(mobj); \
		} \
		else \
		{ \
			mobj->y = value; \
		} \
		break; \
	}

		switch (property)
		{
			PROP_READONLY(THING_PROP_X, x)
			PROP_READONLY(THING_PROP_Y, y)
			PROP_READONLY(THING_PROP_Z, z)
			PROP_TYPE(THING_PROP_TYPE, type)
			PROP_ANGLE(THING_PROP_ANGLE, angle)
			PROP_ANGLE(THING_PROP_PITCH, pitch)
			PROP_ANGLE(THING_PROP_ROLL, roll)
			PROP_ANGLE(THING_PROP_SPRITEROLL, spriteroll)
			PROP_INT(THING_PROP_FRAME, frame)
			PROP_SPR(THING_PROP_SPRITE, sprite)
			PROP_SPR2(THING_PROP_SPRITE2, sprite2)
			PROP_INT(THING_PROP_RENDERFLAGS, renderflags)
			PROP_INT(THING_PROP_SPRITEXSCALE, spritexscale)
			PROP_INT(THING_PROP_SPRITEYSCALE, spriteyscale)
			PROP_INT(THING_PROP_SPRITEXOFFSET, spritexoffset)
			PROP_INT(THING_PROP_SPRITEYOFFSET, spriteyoffset)
			PROP_INT(THING_PROP_FLOORZ, floorz)
			PROP_INT(THING_PROP_CEILINGZ, ceilingz)
			PROP_READONLY(THING_PROP_RADIUS, radius)
			PROP_READONLY(THING_PROP_HEIGHT, height)
			PROP_INT(THING_PROP_MOMX, momx)
			PROP_INT(THING_PROP_MOMY, momy)
			PROP_INT(THING_PROP_MOMZ, momz)
			PROP_INT(THING_PROP_TICS, tics)
			PROP_STATE(THING_PROP_STATE, state)
			PROP_FLAGS(THING_PROP_FLAGS, flags)
			PROP_INT(THING_PROP_FLAGS2, flags2)
			PROP_INT(THING_PROP_EFLAGS, eflags)
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

#undef PROP_FLAGS
#undef PROP_SCALE
#undef PROP_MOBJ
#undef PROP_COLOR
#undef PROP_SKIN
#undef PROP_STATE
#undef PROP_SPR2
#undef PROP_SPR
#undef PROP_TYPE
#undef PROP_ANGLE
#undef PROP_STR
#undef PROP_INT
#undef PROP_READONLY

	}

	thread->dataStk.push(0);

	return false;
}
