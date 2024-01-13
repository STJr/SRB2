// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  usdf.h
/// \brief USDF-SRB2 parsing

#ifndef __USDF_H__
#define __USDF_H__

#include "doomtype.h"

void P_LoadDialogueLumps(UINT16 wadnum);

char *P_ConvertSOCPageDialog(char *text, size_t *text_length);

INT32 P_ParsePromptBackColor(const char *color);

#endif
