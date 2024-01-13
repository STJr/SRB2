// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_writebuffer.h
/// \brief Basic resizeable buffer

#ifndef __M_TEXTBUFFER__
#define __M_TEXTBUFFER__

#include "doomtype.h"

typedef struct
{
	UINT8 *data;
	size_t pos;
	size_t capacity;
} writebuffer_t;

void M_BufferInit(writebuffer_t *buffer);
void M_BufferFree(writebuffer_t *buffer);
void M_BufferWrite(writebuffer_t *buffer, UINT8 chr);
void M_BufferMemWrite(writebuffer_t *buffer, UINT8 *mem, size_t count);

#endif
