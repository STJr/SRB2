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
/// \file  m_argv.h
/// \brief Command line arguments

#ifndef __M_ARGV__
#define __M_ARGV__

//
// MISC
//
extern INT32 myargc;
extern char **myargv;
extern boolean myargmalloc;

// Looks for an srb2:// (or similar) URL passed in as an argument and returns the IP to connect to if found.
const char *M_GetUrlProtocolArg(void);

// Returns the position of the given parameter in the arg list (0 if not found).
INT32 M_CheckParm(const char *check);

// Pushes all parameters beginning with a +, ex: +map map01
void M_PushSpecialParameters(void);

// Returns true if there are available parameters.
boolean M_IsNextParm(void);

// Returns the next parameter after a M_CheckParm, NULL if not found.
const char *M_GetNextParm(void);

// Finds and loads a response file (@moreargs.txt)
void M_FindResponseFile(void);

#endif //__M_ARGV__
