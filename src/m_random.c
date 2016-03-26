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
/// \file  m_random.c
/// \brief RNG for client effects and PRNG for game actions

#include "doomdef.h"
#include "doomtype.h"
#include "doomstat.h" // totalplaytime

#include "m_random.h"
#include "m_fixed.h"

// ---------------------------
// RNG functions (not synched)
// ---------------------------

/** Provides a random byte.
  * Used outside the p_xxx game code and not synchronized in netgames. This is
  * for anything that doesn't need to be synced, e.g. precipitation.
  *
  * \return A random byte, 0 to 255.
  */
UINT8 M_Random(void)
{
	return (rand() & 255);
}

/** Provides a random signed byte.  Distribution is uniform.
  * As with all M_*Random functions, not synched in netgames.
  *
  * \return A random byte, -128 to 127.
  * \sa M_Random
  */
INT32 M_SignedRandom(void)
{
	return (rand() & 255) - 128;
}

/** Provides a random number in between 0 and the given number - 1.
  * Distribution is uniform.  Use for picking random elements from an array.
  * As with all M_*Random functions, not synched in netgames.
  *
  * \return A random number, 0 to arg1-1.
  */
INT32 M_RandomKey(INT32 a)
{
	return (INT32)((rand()/((unsigned)RAND_MAX+1.0f))*a);
}

/** Provides a random number in between a specific range.
  * Distribution is uniform.
  * As with all M_*Random functions, not synched in netgames.
  *
  * \return A random number, arg1 to arg2.
  */
INT32 M_RandomRange(INT32 a, INT32 b)
{
	return (INT32)((rand()/((unsigned)RAND_MAX+1.0f))*(b-a+1))+a;
}



// ------------------------
// PRNG functions (synched)
// ------------------------

// Holds the current seed.
static UINT32 randomseed = 0;

// Holds the INITIAL seed value.  Used for demos, possibly other debugging.
static UINT32 initialseed = 0;

/**
  * Provides a random fixed point number.
  * This is a variant of an xorshift PRNG; state fits in a 32 bit integer structure.
  *
  * \return A random fixed point number from [0,1).
  */
ATTRINLINE static fixed_t FUNCINLINE __internal_prng__(void)
{
	randomseed += 7069;
	randomseed ^= randomseed << 17;
	randomseed ^= randomseed >> 9;
	randomseed *= 373;
	randomseed ^= randomseed << 21;
	randomseed ^= randomseed >> 15;
	return (randomseed&((FRACUNIT-1)<<9))>>9;
}

/** Provides a random integer from 0 to 255.
  * Distribution is uniform.
  * If you're curious, (&0xFF00) >> 8 gives the same result
  * as a fixed point multiplication by 256.
  *
  * \return Random integer from [0, 255].
  * \sa __internal_prng__
  */
#ifndef DEBUGRANDOM
UINT8 P_Random(void)
{
#else
UINT8 P_RandomD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_Random() at: %sp %d\n", rfile, rline);
#endif
	return (UINT8)((__internal_prng__()&0xFF00)>>8);
}

/** Provides a random integer from -128 to 127.
  * Distribution is uniform.
  *
  * \return Random integer from [-128, 127].
  * \sa __internal_prng__
  */
#ifndef DEBUGRANDOM
INT32 P_SignedRandom(void)
{
#else
INT32 P_SignedRandomD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_SignedRandom() at: %sp %d\n", rfile, rline);
#endif
	return (INT32)((__internal_prng__()&0xFF00)>>8) - 128;
}

/**
  * Provides a random fixed point number.
  * Literally a wrapper for the internal PRNG function.
  *
  * \return A random fixed point number from [0,1).
  */
#ifndef DEBUGRANDOM
fixed_t P_RandomFixed(void)
{
#else
UINT8 P_RandomFixedD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_Random() at: %sp %d\n", rfile, rline);
#endif
	return __internal_prng__();
}

/** Provides a random integer for picking random elements from an array.
  * Distribution is uniform.
  *
  * \return A random integer from [0,a).
  * \sa __internal_prng__
  */
#ifndef DEBUGRANDOM
INT32 P_RandomKey(INT32 a)
{
#else
INT32 P_RandomKeyD(const char *rfile, INT32 rline, INT32 a)
{
	CONS_Printf("P_RandomKey() at: %sp %d\n", rfile, rline);
#endif
	return (INT32)((__internal_prng__() * a) >> FRACBITS);
}

/** Provides a random integer in a given range.
  * Distribution is uniform.
  *
  * \return A random integer from [a,b].P_Random
  * \sa __internal_prng__
  */
#ifndef DEBUGRANDOM
INT32 P_RandomRange(INT32 a, INT32 b)
{
#else
INT32 P_RandomRangeD(const char *rfile, INT32 rline, INT32 a, INT32 b)
{
	CONS_Printf("P_RandomRange() at: %sp %d\n", rfile, rline);
#endif
	return (INT32)((__internal_prng__() * (b-a+1)) >> FRACBITS) + a;
}



// ----------------------
// PRNG seeds & debugging
// ----------------------

/** Peeks to see what the next result from the PRNG will be.
  * Used for debugging.
  *
  * \return A 'random' fixed point number from [0,1).
  * \sa __internal_prng__
  */
fixed_t P_RandomPeek(void)
{
	UINT32 r = randomseed;
	fixed_t ret = __internal_prng__();
	randomseed = r;
	return ret;
}

/** Gets the current random seed.  Used by netgame savegames.
  *
  * \return Current random seed.
  * \sa P_SetRandSeed
  */
#ifndef DEBUGRANDOM
UINT32 P_GetRandSeed(void)
{
#else
UINT32 P_GetRandSeedD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_GetRandSeed() at: %sp %d\n", rfile, rline);
#endif
	return randomseed;
}

/** Gets the initial random seed.  Used by demos.
  *
  * \return Initial random seed.
  * \sa P_SetRandSeed
  */
#ifndef DEBUGRANDOM
UINT32 P_GetInitSeed(void)
{
#else
UINT32 P_GetInitSeedD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_GetInitSeed() at: %sp %d\n", rfile, rline);
#endif
	return initialseed;
}

/** Sets the random seed.
  * Used at the beginning of the game, and also for netgames.
  *
  * \param rindex New random index.
  * \sa P_GetRandSeed
  */
#ifndef DEBUGRANDOM
void P_SetRandSeed(UINT32 seed)
{
#else
void P_SetRandSeedD(const char *rfile, INT32 rline, UINT32 seed)
{
	CONS_Printf("P_SetRandSeed() at: %sp %d\n", rfile, rline);
#endif
	randomseed = initialseed = seed;
}

/** Gets a randomized seed for setting the random seed.
  *
  * \sa P_GetRandSeed
  */
UINT32 M_RandomizedSeed(void)
{
	return ((totalplaytime & 0xFFFF) << 16)|(rand() & 0xFFFF);
}
