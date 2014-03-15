// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2012-2014 by Matthew "Inuyasha" Walsh.
// Copyright (C) 1999-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_random.h
/// \brief LCG PRNG originally created for XMOD

#ifndef __M_RANDOM__
#define __M_RANDOM__

#include "doomtype.h"
#include "m_fixed.h"

// M_Random functions pull random numbers of various types that aren't network synced.
// P_Random functions pulls random bytes from a LCG PRNG that is network synced.

// RNG functions
UINT8 M_Random(void);
INT32 M_SignedRandom(void);
INT32 M_RandomKey(INT32 a);
INT32 M_RandomRange(INT32 a, INT32 b);

// PRNG functions
#ifdef DEBUGRANDOM
#define P_Random() P_RandomD(__FILE__, __LINE__)
#define P_SignedRandom() P_SignedRandomD(__FILE__, __LINE__)
#define P_RandomKey(c) P_RandomKeyD(__FILE__, __LINE__, c)
#define P_RandomRange(c, d) P_RandomRangeD(__FILE__, __LINE__, c, d)
UINT8 P_RandomD(const char *rfile, INT32 rline);
INT32 P_SignedRandomD(const char *rfile, INT32 rline);
INT32 P_RandomKeyD(const char *rfile, INT32 rline, INT32 a);
INT32 P_RandomRangeD(const char *rfile, INT32 rline, INT32 a, INT32 b);
#else
UINT8 P_Random(void);
INT32 P_SignedRandom(void);
INT32 P_RandomKey(INT32 a);
INT32 P_RandomRange(INT32 a, INT32 b);
#endif
UINT8 P_RandomPeek(void);

// Working with the seed for PRNG
UINT32 P_GetRandSeed(void);
UINT32 P_GetInitSeed(void);
void P_SetRandSeed(UINT32 seed);
UINT32 M_RandomizedSeed(void);

#endif
