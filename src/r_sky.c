// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_sky.c
/// \brief Sky rendering
///        The SRB2 sky is a texture map like any
///        wall, wrapping around. A 1024 columns equal 360 degrees.
///        The default sky map is 256 columns and repeats 4 times
///        on a 320 screen.

#include "doomdef.h"
#include "doomstat.h"
#include "r_sky.h"
#include "r_local.h"
#include "w_wad.h"
#include "z_zone.h"

#include "p_maputl.h" // P_PointOnLineSide

//
// sky mapping
//

/**	\brief Needed to store the number of the dummy sky flat.
	Used for rendering, as well as tracking projectiles etc.
*/
INT32 skyflatnum;

/**	\brief the lump number of the sky texture
*/
INT32 skytexture;

/**	\brief the horizon line in a 256x128 sky texture
*/
INT32 skytexturemid;

/**	\brief the scale of the sky
*/
fixed_t skyscale;

/** \brief used for keeping track of the current sky
*/
INT32 levelskynum;
INT32 globallevelskynum;

/**	\brief	The R_SetupSkyDraw function

	Called at loadlevel after skytexture is set, or when sky texture changes.

	\warning wallcolfunc should be set at R_ExecuteSetViewSize()
	I don't bother because we don't use low detail anymore

	\return	void
*/
void R_SetupSkyDraw(void)
{
	// the horizon line in a 256x128 sky texture
	skytexturemid = (textures[skytexture]->height/2)<<FRACBITS;

	R_SetSkyScale();
}

/**	\brief	The R_SetSkyScale function

	set the correct scale for the sky at setviewsize

	\return void
*/
void R_SetSkyScale(void)
{
	fixed_t difference = vid.fdupx-(vid.dupx<<FRACBITS);
	skyscale = FixedDiv(fovtan, vid.fdupx+difference);
}
