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
/// \brief LCG PRNG originally created for XMOD

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
	return M_Random() - 128;
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
  * Provides a random byte and sets the seed appropriately.
  * The nature of this PRNG allows it to cycle through about two million numbers
  * before it finally starts repeating numeric sequences.
  * That's more than good enough for our purposes.
  *
  * \return A random byte, 0 to 255.
  */
#ifndef DEBUGRANDOM
UINT8 P_Random(void)
{
#else
UINT8 P_RandomD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_Random() at: %sp %d\n", rfile, rline);
#endif
	randomseed = (randomseed*746151647)+48205429;
	return (UINT8)((randomseed >> 17)&255);
}

/** Provides a random number from -128 to 127.
  * Distribution is uniform.
  *
  * \return Random number from -128 to 127.
  * \sa P_Random
  */
#ifndef DEBUGRANDOM
INT32 P_SignedRandom(void)
{
#else
INT32 P_SignedRandomD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_SignedRandom() at: %sp %d\n", rfile, rline);
#endif
	return P_Random() - 128;
}

/** Provides a random number in between 0 and the given number - 1.
  * Distribution is uniform, also calls for two numbers for bigger output range.
  * Use for picking random elements from an array.
  *
  * \return A random number, 0 to arg1-1.
  * \sa P_Random
  */
#ifndef DEBUGRANDOM
INT32 P_RandomKey(INT32 a)
{
#else
INT32 P_RandomKeyD(const char *rfile, INT32 rline, INT32 a)
{
	CONS_Printf("P_RandomKey() at: %sp %d\n", rfile, rline);
#endif
	INT32 prandom = P_Random(); // note: forcing explicit function call order
	prandom |= P_Random() << 8; // (function call order is not strictly defined)
	return (INT32)((prandom/65536.0f)*a);
}

/** Provides a random number in between a specific range.
  * Distribution is uniform, also calls for two numbers for bigger output range.
  *
  * \return A random number, arg1 to arg2.
  * \sa P_Random
  */
#ifndef DEBUGRANDOM
INT32 P_RandomRange(INT32 a, INT32 b)
{
#else
INT32 P_RandomRangeD(const char *rfile, INT32 rline, INT32 a, INT32 b)
{
	CONS_Printf("P_RandomRange() at: %sp %d\n", rfile, rline);
#endif
	INT32 prandom = P_Random(); // note: forcing explicit function call order
	prandom |= P_Random() << 8; // (function call order is not strictly defined)
	return (INT32)((prandom/65536.0f)*(b-a+1))+a;
}

/** Provides a random byte without saving what the seed would be.
  * Used just to debug the PRNG.
  *
  * \return A 'random' byte, 0 to 255.
  * \sa P_Random
  */
UINT8 P_RandomPeek(void)
{
	UINT32 r = (randomseed*746151647)+48205429;
	return (UINT8)((r >> 17)&255);
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
