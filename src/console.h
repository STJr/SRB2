// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  console.h
/// \brief Console drawing and input

#include "d_event.h"
#include "command.h"
#include "i_threads.h"

void CON_Init(void);

boolean CON_Responder(event_t *ev);

#ifdef HAVE_THREADS
extern I_mutex con_mutex;
#endif

// set true when screen size has changed, to adapt console
extern boolean con_recalc;

extern boolean con_startup;

// top clip value for view render: do not draw part of view hidden by console
extern INT32 con_clipviewtop;

// 0 means console if off, or moving out
extern INT32 con_destlines;

extern INT32 con_clearlines; // lines of top of screen to refresh
extern boolean con_hudupdate; // hud messages have changed, need refresh
extern UINT32 con_scalefactor; // console text scale factor

extern consvar_t cons_backcolor;

extern UINT8 *yellowmap, *magentamap, *lgreenmap, *bluemap, *graymap, *redmap, *orangemap, *skymap, *purplemap, *aquamap, *peridotmap, *azuremap, *brownmap, *rosymap, *invertmap;

// Console bg color (auto updated to match)
extern UINT8 *consolebgmap;
extern UINT8 *promptbgmap;

void CON_SetupBackColormapEx(INT32 color, boolean prompt);
void CON_SetupBackColormap(void);
void CON_ClearHUD(void); // clear heads up messages

void CON_Ticker(void);
void CON_Drawer(void);
void CONS_Error(const char *msg); // print out error msg, and wait a key

// force console to move out
void CON_ToggleOff(void);

// Is console down?
boolean CON_Ready(void);

void CON_LogMessage(const char *msg);
