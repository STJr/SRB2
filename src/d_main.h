// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_main.h
/// \brief game startup, and main loop code, system specific interface stuff.

#ifndef __D_MAIN__
#define __D_MAIN__

#include "d_event.h"
#include "w_wad.h"   // for MAX_WADFILES

extern boolean advancedemo;

// make sure not to write back the config until it's been correctly loaded
extern tic_t rendergametic;

extern char srb2home[256]; //Alam: My Home
extern boolean usehome; //Alam: which path?
extern const char *pandf; //Alam: how to path?
extern char srb2path[256]; //Alam: SRB2's Home

// the infinite loop of D_SRB2Loop() called from win_main for windows version
void D_SRB2Loop(void) FUNCNORETURN;

//
// D_SRB2Main()
// Not a globally visible function, just included for source reference,
// calls all startup code, parses command line options.
// If not overrided by user input, calls D_AdvanceDemo.
//
void D_SRB2Main(void);

// Called by IO functions when input is detected.
void D_PostEvent(const event_t *ev);
#if defined (PC_DOS) && !defined (DOXYGEN)
void D_PostEvent_end(void);    // delimiter for locking memory
#endif

void D_ProcessEvents(void);

const char *D_Home(void);

//
// BASE LEVEL
//
void D_AdvanceDemo(void);
void D_StartTitle(void);

#endif //__D_MAIN__
