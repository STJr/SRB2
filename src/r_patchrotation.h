// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_patchrotation.h
/// \brief Patch rotation.

#include "r_patch.h"
#include "r_picformats.h"

#ifdef ROTSPRITE
rotsprite_t *RotatedPatch_Create(INT32 numangles);
void RotatedPatch_DoRotation(rotsprite_t *rotsprite, patch_t *patch, INT32 angle, INT32 xpivot, INT32 ypivot, boolean flip);

extern fixed_t rollcosang[ROTANGLES];
extern fixed_t rollsinang[ROTANGLES];
#endif
