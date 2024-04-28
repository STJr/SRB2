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
/// \file  stream.cpp
/// \brief Action Code Script: Dummy stream buffer to
///        interact with P_Save/LoadNetGame

// TODO? Maybe untie this file from ACS?

#include <istream>
#include <ostream>
#include <streambuf>

#include "../doomtype.h"
#include "../doomdef.h"
#include "../doomstat.h"

#include "../p_saveg.h"

#include "stream.hpp"
#include "../cxxutil.hpp"

using namespace srb2::acs;

SaveBuffer::SaveBuffer(savebuffer_t *save_) :
	save{save_}
{
}

SaveBuffer::int_type SaveBuffer::overflow(SaveBuffer::int_type ch)
{
	if (save->p == save->end)
	{
		return traits_type::eof();
	}

	*save->p = static_cast<UINT8>(ch);
	save->p++;

	return ch;
}

SaveBuffer::int_type SaveBuffer::underflow()
{
	if (save->p == save->end)
	{
		return traits_type::eof();
	}

	UINT8 ret = *save->p;
	save->p++;

	// Allow the streambuf internal funcs to work
	buf[0] = ret;
	setg(
		reinterpret_cast<SaveBuffer::char_type *>(buf),
		reinterpret_cast<SaveBuffer::char_type *>(buf),
		reinterpret_cast<SaveBuffer::char_type *>(buf + 1)
	);

	return static_cast<SaveBuffer::int_type>(ret);
}
