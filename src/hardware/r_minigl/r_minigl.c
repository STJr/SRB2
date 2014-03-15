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
/// \brief MiniGL API for Doom Legacy


// tell r_opengl.cpp to compile for MiniGL Drivers
#define MINI_GL_COMPATIBILITY

// tell r_opengl.cpp to compile for ATI Rage Pro OpenGL driver
//#define ATI_RAGE_PRO_COMPATIBILITY

#define DRIVER_STRING "HWRAPI Init(): SRB2 MiniGL renderer"

// Include this at end
#include "../r_opengl/r_opengl.c"
#include "../r_opengl/ogl_win.c"

// That's all ;-)
// Just, be sure to do the right changes in r_opengl.cpp
