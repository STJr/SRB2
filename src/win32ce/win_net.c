// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief Win32 network interface

#include "../doomdef.h"
#include "../m_argv.h"
#include "../i_net.h"

//
// NETWORKING
//

//
// I_InitNetwork
//
boolean I_InitNetwork (void)
{
	if (M_CheckParm ("-net"))
	{
		I_Error("The Win32 version of SRB2 doesn't work with external drivers like ipxsetup, sersetup, or doomatic\n"
		        "Read the documentation about \"-server\" and \"-connect\" parameters or just use the launcher\n");
	}

	return false;
}
