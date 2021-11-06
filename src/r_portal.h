// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_portal.h
/// \brief Software renderer portal struct, functions, linked list extern.

#ifndef __R_PORTAL__
#define __R_PORTAL__

#include "r_data.h"
#include "r_textures.h"
#include "r_plane.h" // visplanes

/** Portal structure for the software renderer.
 */
typedef struct portal_s
{
	struct portal_s *next;

	// Viewport.
	fixed_t viewx;
	fixed_t viewy;
	fixed_t viewz;
	angle_t viewangle;

	UINT8 pass;			/**< Keeps track of the portal's recursion depth. */
	INT32 clipline;		/**< Optional clipline for line-based portals. */

	// Clipping information.
	INT32 start;		/**< First horizontal pixel coordinate to draw at. */
	INT32 end;			/**< Last horizontal pixel coordinate to draw at. */
	INT16 *ceilingclip; /**< Temporary screen top clipping array. */
	INT16 *floorclip;	/**< Temporary screen bottom clipping array. */
	fixed_t *frontscale;/**< Temporary screen bottom clipping array. */
} portal_t;

struct bspcontext_s;
struct planecontext_s;
struct rendercontext_s;
struct viewcontext_s;

void Portal_InitList	(struct bspcontext_s *bspcontext);
void Portal_Remove		(struct bspcontext_s *context, portal_t* portal);
void Portal_Add2Lines	(struct rendercontext_s *context,
                        const INT32 line1, const INT32 line2, const INT32 x1, const INT32 x2);
void Portal_AddSkybox	(struct bspcontext_s *bspcontext, struct viewcontext_s *viewcontext, const visplane_t* plane);

void Portal_ClipRange (struct planecontext_s *planecontext, portal_t* portal);
void Portal_ClipApply (struct planecontext_s *planecontext, const portal_t* portal);

void Portal_AddSkyboxPortals (struct rendercontext_s *context);
#endif
