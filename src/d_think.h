// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
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

#ifdef __GNUG__
#pragma interface
#endif

//
// Experimental stuff.
// To compile this as "ANSI C with classes" we will need to handle the various
//  action functions cleanly.
//
typedef void (*actionf_v)();
typedef void (*actionf_p1)(void *);

typedef union
{
	actionf_v acv;
	actionf_p1 acp1;
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
} thinker_t;

#endif
