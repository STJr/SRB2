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
/// \file  r_segs.h
/// \brief Refresh module, drawing LineSegs from BSP

#ifndef __R_SEGS__
#define __R_SEGS__

#ifdef __GNUG__
#pragma interface
#endif

struct rendercontext_s;
struct wallcontext_s;

transnum_t R_GetLinedefTransTable(fixed_t alpha);
void R_RenderMaskedSegRange(struct rendercontext_s *context, drawseg_t *ds, INT32 x1, INT32 x2);
void R_RenderThickSideRange(struct rendercontext_s *context, drawseg_t *ds, INT32 x1, INT32 x2, ffloor_t *pffloor);
void R_StoreWallRange(struct rendercontext_s *context, struct wallcontext_s *wallcontext, INT32 start, INT32 stop);

#endif
