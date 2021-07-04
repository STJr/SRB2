// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2012-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_random.h
/// \brief RNG for client effects and PRNG for game actions

#ifndef __M_RANDOM__
#define __M_RANDOM__

#include "doomtype.h"
#include "m_fixed.h"

//#define DEBUGRANDOM


// M_Random functions pull random numbers of various types that aren't network synced.
// P_Random functions pulls random bytes from a PRNG that is network synced.

// RNG functions
fixed_t M_RandomFixed(void);
UINT8   M_RandomByte(void);
INT32   M_RandomKey(INT32 a);
INT32   M_RandomRange(INT32 a, INT32 b);

// PRNG functions
#ifdef DEBUGRANDOM
#define P_RandomFixed()     P_RandomFixedD(__FILE__, __LINE__)
#define P_RandomByte()      P_RandomByteD(__FILE__, __LINE__)
#define P_RandomKey(c)      P_RandomKeyD(__FILE__, __LINE__, c)
#define P_RandomRange(c, d) P_RandomRangeD(__FILE__, __LINE__, c, d)
fixed_t P_RandomFixedD(const char *rfile, INT32 rline);
UINT8   P_RandomByteD(const char *rfile, INT32 rline);
INT32   P_RandomKeyD(const char *rfile, INT32 rline, INT32 a);
INT32   P_RandomRangeD(const char *rfile, INT32 rline, INT32 a, INT32 b);
#else
fixed_t P_RandomFixed(void);
UINT8   P_RandomByte(void);
INT32   P_RandomKey(INT32 a);
INT32   P_RandomRange(INT32 a, INT32 b);
#endif

// Macros for other functions
#define M_SignedRandom()  ((INT32)M_RandomByte() - 128) // [-128, 127] signed byte, originally a
#define P_SignedRandom()  ((INT32)P_RandomByte() - 128) // function of its own, moved to a macro

#define M_RandomChance(p) (M_RandomFixed() < p) // given fixed point probability, p, between 0 (0%)
#define P_RandomChance(p) (P_RandomFixed() < p) // and FRACUNIT (100%), returns true p% of the time

// Debugging
fixed_t P_RandomPeek(void);

// Working with the seed for PRNG
#ifdef DEBUGRANDOM
#define P_GetRandSeed() P_GetRandSeedD(__FILE__, __LINE__)
#define P_GetInitSeed() P_GetInitSeedD(__FILE__, __LINE__)
#define P_SetRandSeed(s) P_SetRandSeedD(__FILE__, __LINE__, s)
UINT32 P_GetRandSeedD(const char *rfile, INT32 rline);
UINT32 P_GetInitSeedD(const char *rfile, INT32 rline);
void P_SetRandSeedD(const char *rfile, INT32 rline, UINT32 seed);
#else
UINT32 P_GetRandSeed(void);
UINT32 P_GetInitSeed(void);
void P_SetRandSeed(UINT32 seed);
#endif
UINT32 M_RandomizedSeed(void);

#endif
