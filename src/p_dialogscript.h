// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_dialogscript.h
/// \brief Text prompt script interpreter

#ifndef __P_DIALOGSCRIPT__
#define __P_DIALOGSCRIPT__

#include "doomtype.h"

#include "p_dialog.h"

#include "m_writebuffer.h"

enum
{
	TP_OP_SPEED,
	TP_OP_DELAY,
	TP_OP_PAUSE,

	TP_OP_NEXTPAGE,

	TP_OP_NAME,
	TP_OP_ICON,

	TP_OP_CHARNAME,
	TP_OP_PLAYERNAME,

	TP_OP_CONTROL = 0xFF
};

const UINT8 *P_DialogRunOpcode(const UINT8 *code, dialog_t *dialog, textwriter_t *writer);
boolean P_DialogPreprocessOpcode(dialog_t *dialog, UINT8 **cptr, writebuffer_t *buf);
int P_DialogSkipOpcode(const UINT8 *code);

#endif
