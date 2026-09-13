// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  simplehash.h
/// \brief Macros for handling basic hashmap types

#ifndef __SIMPLEHASH__
#define __SIMPLEHASH__

#include "uthash.h"

typedef struct
{
	INT32 k;
	INT32 v;
	UT_hash_handle hh;
} hashentry_int32_int32_t;

// hashmap<type>[key] = value;
#define SIMPLEHASH_REPLACE_INT(hashmap, type, key, value)      \
{                                                              \
	type *entry = malloc(sizeof(type));                        \
	if (!entry)                                                \
		I_Error("%s:%d: Out of memory\n", __func__, __LINE__); \
	entry->k = (key);                                          \
	entry->v = (value);                                        \
                                                               \
	type *oldentry;                                            \
	HASH_REPLACE_INT((hashmap), k, entry, oldentry);           \
	if (oldentry)                                              \
		free(oldentry);                                        \
}

#define SIMPLEHASH_CLEAR(hashmap, type)       \
{                                             \
	type *entry, *tmpentry;                   \
	HASH_ITER(hh, (hashmap), entry, tmpentry) \
	{                                         \
		HASH_DEL((hashmap), entry);           \
		free(entry);                          \
	}                                         \
}

// value = hashmap<type>[key] or fallback;
#define SIMPLEHASH_FIND_INT(hashmap, type, key, fallback, value) \
{                                                                \
	int tmpkey = (key);                                          \
	type *entry;                                                 \
	HASH_FIND_INT((hashmap), &tmpkey, entry);                    \
	(value) = entry ? entry->v : (int)(fallback);                \
}

#endif //__SIMPLEHASH__
