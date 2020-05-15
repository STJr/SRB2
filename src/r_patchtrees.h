// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_patchtrees.h
/// \brief Patch tree definitions.

#ifndef __R_PATCHTREES__
#define __R_PATCHTREES__

#include "m_aatree.h"

// Renderer tree types
typedef enum patchtreetype_e
{
	patchtree_software, // Software.
	patchtree_mipmap, // OpenGL, or any hardware renderer.

	num_patchtrees,
} patchtreetype_t;

// Renderer patch trees
typedef struct patchtree_s
{
	aatree_t *base;
#ifdef ROTSPRITE
	aatree_t *rotated[2]; // Sprite rotation stores flipped and non-flipped variants of a patch.
#endif
} patchtree_t;

#endif // __R_PATCHTREES__
