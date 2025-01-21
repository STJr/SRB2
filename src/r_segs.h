// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
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

transnum_t R_GetLinedefTransTable(fixed_t alpha);
void R_RenderMaskedSegRange(drawseg_t *ds, INT32 x1, INT32 x2);
void R_RenderThickSideRange(drawseg_t *ds, INT32 x1, INT32 x2, ffloor_t *pffloor);
void R_StoreWallRange(INT32 start, INT32 stop);
void R_ClearSegTables(void);

#endif
