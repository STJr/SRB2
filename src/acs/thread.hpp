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
/// \file  thread.hpp
/// \brief Action Code Script: Thread definition

#ifndef __SRB2_ACS_THREAD_HPP__
#define __SRB2_ACS_THREAD_HPP__

#include "acsvm.hpp"

extern "C" {
#include "../doomtype.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../p_tick.h"
#include "../r_defs.h"
#include "../r_state.h"
#include "../p_spec.h"
}

namespace srb2::acs {

//
// Special global script types.
//
enum acs_scriptType_e
{
	ACS_ST_OPEN         =  1,  // OPEN: Runs once when the level starts.
	ACS_ST_RESPAWN      =  2,  // RESPAWN: Runs when a player respawns.
	ACS_ST_DEATH        =  3,  // DEATH: Runs when a player dies.
	ACS_ST_ENTER        =  4,  // ENTER: Runs when a player enters the game; both on start of the level, and when un-spectating.
	ACS_ST_PICKUP       =  5,  // PICKUP: Runs when a player picks up a flag. (Skulltag)
	ACS_ST_BLUERETURN   =  6,  // BLUERETURN: Runs when the blue flag returns to its base. (Skulltag)
	ACS_ST_REDRETURN    =  7,  // REDRETURN: Runs when the red flag returns to its base. (Skulltag)
	ACS_ST_EVENT        =  16, // EVENT: Runs when certain game events happen. (Zandronum)
	ACS_ST_GAMEOVER	    =  19, // GAMEOVER: Runs when the player gets a game over. (SRB2)
	ACS_ST_FINISH       =  20  // FINISH: Runs when a player finishes the level. (SRB2)
};

//
// Script "waiting on tag" types.
//
enum acs_tagType_e
{
	ACS_TAGTYPE_POLYOBJ,
	ACS_TAGTYPE_SECTOR,
	ACS_TAGTYPE_CAMERA
};

class ThreadInfo : public ACSVM::ThreadInfo
{
public:
	UINT32 thread_era;			// If equal to thinker_era, mobj pointers are safe.
	mobj_t *mo;					// Object that activated this thread.
	line_t *line;				// Linedef that activated this thread.
	UINT8 side;					// Front / back side of said linedef.
	sector_t *sector;			// Sector that activated this thread.
	polyobj_t *po;				// Polyobject that activated this thread.
	bool fromLineSpecial;		// Called from P_ProcessLineSpecial.

	ThreadInfo() :
		thread_era { thinker_era },
		mo{ nullptr },
		line{ nullptr },
		side{ 0 },
		sector{ nullptr },
		po{ nullptr },
		fromLineSpecial{ false }
	{
	}

	ThreadInfo(const ThreadInfo &info) :
		thread_era { thinker_era },
		mo{ nullptr },
		line{ info.line },
		side{ info.side },
		sector{ info.sector },
		po{ info.po },
		fromLineSpecial{ info.fromLineSpecial }
	{
		P_SetTarget(&mo, info.mo);
	}

	ThreadInfo(const activator_t *activator) :
		thread_era { thinker_era },
		mo{ nullptr },
		line{ activator->line },
		side{ activator->side },
		sector{ activator->sector },
		po{ activator->po },
		fromLineSpecial{ static_cast<bool>(activator->fromLineSpecial) }
	{
		P_SetTarget(&mo, activator->mo);
	}

	~ThreadInfo()
	{
		if (thread_era == thinker_era)
		{
			P_SetTarget(&mo, nullptr);
		}
	}

	ThreadInfo &operator = (const ThreadInfo &info)
	{
		thread_era = thinker_era;
		P_SetTarget(&mo, info.mo);
		line = info.line;
		side = info.side;
		sector = info.sector;
		po = info.po;

		return *this;
	}
};

class Thread : public ACSVM::Thread
{
public:
	ThreadInfo info;

	explicit Thread(ACSVM::Environment *env_) : ACSVM::Thread{env_} {}

	virtual ACSVM::ThreadInfo const *getInfo() const { return &info; }

	virtual void start(
		ACSVM::Script *script, ACSVM::MapScope *map,
		const ACSVM::ThreadInfo *info, const ACSVM::Word *argV, ACSVM::Word argC
	);

	virtual void stop();

	virtual void loadState(ACSVM::Serial &serial);

	virtual void saveState(ACSVM::Serial &serial) const;
};

}

#endif // __SRB2_ACS_THREAD_HPP__
