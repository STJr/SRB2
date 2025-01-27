// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2012-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 2022-2023 by tertu marybig.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_random.c
/// \brief RNG for client effects and PRNG for game actions

#include "doomdef.h"
#include "doomtype.h"
#include "i_system.h" // I_GetRandomBytes

#include "m_random.h"
#include "m_fixed.h"

// SFC32 random number generator implementation

typedef struct rnstate_s {
	UINT32 data[3];
	UINT32 counter;
} rnstate_t;

/** Generate a raw uniform random number using a particular state.
  *
  * \param state The RNG state to use.
  * \return A random UINT32.
  */
static inline UINT32 RandomState_Get32(rnstate_t *state) {
	UINT32 result, b, c;

	b = state->data[1];
	c = state->data[2];
	result = state->data[0] + b + state->counter++;

	state->data[0] = b ^ (b >> 9);
	state->data[1] = c * 9;
	state->data[2] = ((c << 21) | (c >> 11)) + result;

	return result;
}

/** Seed an SFC32 RNG state with up to 96 bits of seed data.
  *
  * \param state The RNG state to seed.
  * \param seeds A pointer to up to 3 UINT32s to use as seed data.
  * \param seed_count The number of seed words.
  */
static inline void RandomState_Seed(rnstate_t *state, UINT32 *seeds, size_t seed_count)
{
	size_t i;

	state->counter = 1;

	for(i = 0; i < 3; i++)
	{
		UINT32 seed_word;

		if(i < seed_count)
			seed_word = seeds[i];
		else
			seed_word = 0;

		// For SFC32, seed data should be stored in the state in reverse order.
		state->data[2-i] = seed_word;
	}

	for(i = 0; i < 16; i++)
		RandomState_Get32(state);
}

/** Gets a uniform number in the range [0, limit).
  * Technique is based on a combination of scaling and rejection sampling
  * and is adapted from Daniel Lemire.
  *
  * \note Any UINT32 is a valid argument for limit.
  *
  * \param state The RNG state to use.
  * \param limit The upper limit of the range.
  * \return A UINT32 in the range [0, limit).
  */
static inline UINT32 RandomState_GetKey32(rnstate_t *state, const UINT32 limit)
{
	UINT32 raw_random, scaled_lower_word;
	UINT64 scaled_random;

	// This algorithm won't work correctly if passed a 0.
	if (limit == 0) return 0;

	raw_random = RandomState_Get32(state);
	scaled_random = (UINT64)raw_random * (UINT64)limit;

	/*The high bits of scaled_random now contain the number we want, but it is
	possible, depending on the number we generated and the value of limit,
	that there is bias in the result. The rest of this code is for ensuring
	that does not happen.
	*/
	scaled_lower_word = (UINT32)scaled_random;

	// If we're lucky, we can bail out now and avoid the division
	if (scaled_lower_word < limit)
	{
		// Scale the limit to improve the chance of success.
		// After this, the first result might turn out to be good enough.
		UINT32 scaled_limit;
		// An explanation for this trick: scaled_limit should be
		// (UINT32_MAX+1)%range, but if that was computed directly the result
		// would need to be computed as a UINT64. This trick allows it to be
		// computed using 32-bit arithmetic.
		scaled_limit = (-limit) % limit;

		while (scaled_lower_word < scaled_limit)
		{
			raw_random = RandomState_Get32(state);
			scaled_random = (UINT64)raw_random * (UINT64)limit;
			scaled_lower_word = (UINT32)scaled_random;
		}
	}

	return scaled_random >> 32;
}

// The default seed is the hexadecimal digits of pi, though it will be overwritten.
static rnstate_t m_randomstate = {
	.data = {0x4A3B6035U, 0x99555606U, 0x6F603421U},
	.counter = 16
};

// ---------------------------
// RNG functions (not synched)
// ---------------------------

/** Provides a random fixed point number. Distribution is uniform.
  * As with all M_Random functions, not synched in netgames.
  *
  * \return A random fixed point number from [0,1).
  */
fixed_t M_RandomFixed(void)
{
	return RandomState_Get32(&m_randomstate) >> (32-FRACBITS);
}

/** Provides a random byte. Distribution is uniform.
  * As with all M_Random functions, not synched in netgames.
  *
  * \return A random integer from [0, 255].
  */
UINT8 M_RandomByte(void)
{
	return RandomState_Get32(&m_randomstate) >> 24;
}

/** Provides a random integer for picking random elements from an array.
  * Distribution is uniform.
  * As with all M_Random functions, not synched in netgames.
  *
  * \param a Number of items in array.
  * \return A random integer from [0,a).
  */
INT32 M_RandomKey(INT32 a)
{
	boolean range_is_negative;
	INT64 range;
	INT32 random_result;

	range = a;
	range_is_negative = range < 0;

	if(range_is_negative)
		range = -range;

	random_result = RandomState_GetKey32(&m_randomstate, (UINT32)range);

	if(range_is_negative)
		random_result = -random_result;

	return random_result;
}

/** Provides a random integer in a given range.
  * Distribution is uniform.
  * As with all M_Random functions, not synched in netgames.
  *
  * \param a Lower bound.
  * \param b Upper bound.
  * \return A random integer from [a,b].
  */
INT32 M_RandomRange(INT32 a, INT32 b)
{
	if (b < a)
	{
		INT32 temp;

		temp = a;
		a = b;
		b = temp;
	}

	const UINT32 spread = b-a+1;
	return (INT32)((INT64)RandomState_GetKey32(&m_randomstate, spread) + a);
}

/** Attempts to seed the unsynched RNG from a good random number source
  * provided by the operating system.
  * \return true on success, false on failure.
  */
boolean M_RandomSeedFromOS(void)
{
	UINT32 complete_word_count;

	union {
		UINT32 words[3];
		char bytes[sizeof(UINT32[3])];
	} seed_data;

	complete_word_count = I_GetRandomBytes((char *)&seed_data.bytes, sizeof(seed_data)) / sizeof(UINT32);

	// If we get even 1 word of seed, it's fine, but any less probably is not fine.
	if (complete_word_count == 0)
		return false;

	RandomState_Seed(&m_randomstate, (UINT32 *)&seed_data.words, complete_word_count);

	return true;
}

void M_RandomSeed(UINT32 seed)
{
	RandomState_Seed(&m_randomstate, &seed, 1);
}



// ------------------------
// PRNG functions (synched)
// ------------------------

// Holds the current seed.
static UINT32 randomseed = 0xBADE4404;

// Holds the INITIAL seed value.  Used for demos, possibly other debugging.
static UINT32 initialseed = 0xBADE4404;

/** Provides a random fixed point number.
  * This is a variant of an xorshift PRNG; state fits in a 32 bit integer structure.
  *
  * \return A random fixed point number from [0,1).
  */
ATTRINLINE static fixed_t FUNCINLINE __internal_prng__(void)
{
	randomseed ^= randomseed >> 13;
	randomseed ^= randomseed >> 11;
	randomseed ^= randomseed << 21;
	return ( (randomseed*36548569) >> 4) & (FRACUNIT-1);
}

/** Provides a random fixed point number. Distribution is uniform.
  * Literally a wrapper for the internal PRNG function.
  *
  * \return A random fixed point number from [0,1).
  */
#ifndef DEBUGRANDOM
fixed_t P_RandomFixed(void)
{
#else
fixed_t P_RandomFixedD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_RandomFixed() at: %sp %d\n", rfile, rline);
#endif
	return __internal_prng__();
}

/** Provides a random byte. Distribution is uniform.
  * If you're curious, (&0xFF00) >> 8 gives the same result
  * as a fixed point multiplication by 256.
  *
  * \return Random integer from [0, 255].
  * \sa __internal_prng__
  */
#ifndef DEBUGRANDOM
UINT8 P_RandomByte(void)
{
#else
UINT8 P_RandomByteD(const char *rfile, INT32 rline)
{
	CONS_Printf("P_RandomByte() at: %sp %d\n", rfile, rline);
#endif
	return (UINT8)((__internal_prng__()&0xFF00)>>8);
}

/** Provides a random integer for picking random elements from an array.
  * Distribution is uniform.
  * NOTE: Maximum range is 65536.
  *
  * \param a Number of items in array.
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
	return (INT32)(((INT64)__internal_prng__() * a) >> FRACBITS);
}

/** Provides a random integer in a given range.
  * Distribution is uniform.
  * NOTE: Maximum range is 65536.
  *
  * \param a Lower bound.
  * \param b Upper bound.
  * \return A random integer from [a,b].
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
	return (INT32)(((INT64)__internal_prng__() * (b-a+1)) >> FRACBITS) + a;
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
	// xorshift requires a nonzero seed
	// this should never happen, but just in case it DOES, we check
	if (!seed) seed = 0xBADE4404;
	randomseed = initialseed = seed;
}

/** Gets a randomized seed for setting the random seed.
  * This function will never return 0, as the current P_Random implementation
  * cannot handle a zero seed. Any other seed is equally likely.
  *
  * \sa P_GetRandSeed
  */
UINT32 M_RandomizedSeed(void)
{
	UINT32 seed;

	do {
		seed = RandomState_Get32(&m_randomstate);
	} while(seed == 0);

	return seed;
}
