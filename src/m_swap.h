// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_swap.h
/// \brief Endianess handling, swapping 16bit and 32bit

#ifndef __M_SWAP__
#define __M_SWAP__

// Endianess handling.
// WAD files are stored little endian.
#ifdef _BIG_ENDIAN

#define SHORT(x) ((INT16)(\
(((UINT16)(x) & (UINT16)0x00ffU) << 8) \
| \
(((UINT16)(x) & (UINT16)0xff00U) >> 8))) \

#define LONG(x) ((INT32)(\
(((UINT32)(x) & (UINT32)0x000000ffUL) << 24) \
| \
(((UINT32)(x) & (UINT32)0x0000ff00UL) <<  8) \
| \
(((UINT32)(x) & (UINT32)0x00ff0000UL) >>  8) \
| \
(((UINT32)(x) & (UINT32)0xff000000UL) >> 24)))

#else
#define SHORT(x) ((INT16)(x))
#define LONG(x)	((INT32)(x))
#endif

#endif
