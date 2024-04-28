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
/// \file  stream.hpp
/// \brief Action Code Script: Dummy stream buffer to
///        interact with P_Save/LoadNetGame

// TODO? Maybe untie this file from ACS?

#ifndef __SRB2_ACS_STREAM_HPP__
#define __SRB2_ACS_STREAM_HPP__

#include <streambuf>

#include "acsvm.hpp"

extern "C" {
#include "../doomtype.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../p_saveg.h"
}

namespace srb2::acs {

class SaveBuffer : public std::streambuf
{
public:
	savebuffer_t *save;
	UINT8 buf[1];

	explicit SaveBuffer(savebuffer_t *save_);

private:
	virtual int_type overflow(int_type ch);
	virtual int_type underflow();
};

}

#endif // __SRB2_ACS_STREAM_HPP__
