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
/// \file  thread.cpp
/// \brief Action Code Script: Thread definition

#include "acsvm.hpp"

#include "thread.hpp"

#include "../doomtype.h"
#include "../doomdef.h"
#include "../doomstat.h"

#include "../p_saveg.h"
#include "../p_tick.h"
#include "../p_local.h"
#include "../r_defs.h"
#include "../r_state.h"
#include "../p_polyobj.h"

using namespace srb2::acs;

void Thread::start(
	ACSVM::Script *script, ACSVM::MapScope *map,
	const ACSVM::ThreadInfo *infoPtr, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::Thread::start(script, map, infoPtr, argV, argC);

	if (infoPtr != nullptr)
	{
		info = *static_cast<const ThreadInfo *>(infoPtr);
	}
	else
	{
		info = {};
	}

	result = 1;
}

void Thread::stop()
{
	ACSVM::Thread::stop();
	info = {};
}

void Thread::saveState(ACSVM::Serial &serial) const
{
	ACSVM::Thread::saveState(serial);

	ACSVM::WriteVLN<size_t>(serial, (info.mo != nullptr && P_MobjWasRemoved(info.mo) == false) ? (info.mo->mobjnum) : 0);
	ACSVM::WriteVLN<size_t>(serial, (info.line != nullptr) ? ((info.line - lines) + 1) : 0);
	ACSVM::WriteVLN<size_t>(serial, info.side);
	ACSVM::WriteVLN<size_t>(serial, (info.sector != nullptr) ? ((info.sector - sectors) + 1) : 0);
	ACSVM::WriteVLN<size_t>(serial, (info.po != nullptr) ? ((info.po - PolyObjects) + 1) : 0);
}

void Thread::loadState(ACSVM::Serial &serial)
{
	ACSVM::Thread::loadState(serial);

	UINT32 temp = static_cast<UINT32>(ACSVM::ReadVLN<size_t>(serial));

	if (temp != 0)
	{
		info.mo = nullptr;

		if (P_SetTarget(&info.mo, P_FindNewPosition(temp)) == nullptr)
		{
			CONS_Debug(DBG_GAMELOGIC, "info.mo not found for ACS thread\n"); // todo: identify which thread
		}
	}

	size_t lineIndex = ACSVM::ReadVLN<size_t>(serial);
	info.line = (lineIndex != 0) ? (&lines[lineIndex - 1]) : nullptr;

	info.side = static_cast<UINT8>(ACSVM::ReadVLN<size_t>(serial));

	size_t sectorIndex = ACSVM::ReadVLN<size_t>(serial);
	info.sector = (sectorIndex != 0) ? (&sectors[sectorIndex - 1]) : nullptr;

	size_t polyIndex = ACSVM::ReadVLN<size_t>(serial);
	info.po = (polyIndex != 0) ? (&PolyObjects[polyIndex - 1]) : nullptr;
}
