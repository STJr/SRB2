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
/// \file  d_think.h
/// \brief MapObj data. Map Objects or mobjs are actors, entities,
///        thinker, take-your-pick
///
///        anything that moves, acts, or suffers state changes of more or less violent nature.

#ifndef __D_THINK__
#define __D_THINK__

#include "doomdef.h"

#ifdef __GNUG__
#pragma interface
#endif

enum
{
	ACTION_VAL_NULL,
	ACTION_VAL_INTEGER,
	ACTION_VAL_BOOLEAN,
	ACTION_VAL_STRING
};

typedef struct
{
	unsigned length;
	char *chars;
	UINT32 hash;
} action_string_t;

typedef struct
{
	UINT8 type;
	union
	{
		INT32 v_integer;
		boolean v_bool;
		UINT32 v_string_id;
	};
} action_val_t;

#define ACTION_VAL_IS_NULL(val) ((val).type == ACTION_VAL_NULL)
#define ACTION_VAL_IS_INTEGER(val) ((val).type == ACTION_VAL_INTEGER)
#define ACTION_VAL_IS_BOOLEAN(val) ((val).type == ACTION_VAL_BOOLEAN)
#define ACTION_VAL_IS_STRING(val) ((val).type == ACTION_VAL_STRING)

#define ACTION_NULL_VAL (action_val_t){ .type = ACTION_VAL_NULL }
#define ACTION_INTEGER_VAL(val) (action_val_t){ .type = ACTION_VAL_INTEGER, .v_integer = (val) }
#define ACTION_BOOLEAN_VAL(val) (action_val_t){ .type = ACTION_VAL_BOOLEAN, .v_bool = (val) }
#define ACTION_STRING_VAL(val) (action_val_t){ .type = ACTION_VAL_STRING, .v_string_id = (val) }

#define ACTION_VAL_AS_INTEGER(val) ((val).v_integer)
#define ACTION_VAL_AS_BOOLEAN(val) ((val).v_bool)
#define ACTION_VAL_AS_STRING(val) ((val).v_string_id)

//
// Experimental stuff.
// To compile this as "ANSI C with classes" we will need to handle the various
//  action functions cleanly.
//
typedef void (*actionf_v)();
typedef void (*actionf_p1)(void *);
typedef void (*actionf_script)(void *, action_val_t*, unsigned);

typedef union
{
	actionf_v acv;
	actionf_p1 acp1;
	actionf_script acpscr;
} actionf_t;

// Historically, "think_t" is yet another function pointer to a routine
// to handle an actor.
typedef actionf_t think_t;

// Doubly linked list of actors.
typedef struct thinker_s
{
	struct thinker_s *prev;
	struct thinker_s *next;
	think_t function;

	// killough 11/98: count of how many other objects reference
	// this one using pointers. Used for garbage collection.
	INT32 references;

#ifdef PARANOIA
	INT32 debug_mobjtype;
	tic_t debug_time;
#endif
} thinker_t;

#endif
