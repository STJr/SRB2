// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_writebuffer.c
/// \brief Basic resizeable buffer

#include "m_writebuffer.h"

#include "z_zone.h"

void M_BufferInit(writebuffer_t *buffer)
{
	buffer->data = NULL;
	buffer->pos = 0;
	buffer->capacity = 16;
}

void M_BufferFree(writebuffer_t *buffer)
{
	Z_Free(buffer->data);
	M_BufferInit(buffer);
}

void M_BufferWrite(writebuffer_t *buffer, UINT8 chr)
{
	if (!buffer->data || buffer->pos >= buffer->capacity)
	{
		buffer->capacity *= 2;
		buffer->data = Z_Realloc(buffer->data, buffer->capacity, PU_STATIC, NULL);
	}

	buffer->data[buffer->pos] = chr;
	buffer->pos++;
}

void M_BufferMemWrite(writebuffer_t *buffer, UINT8 *mem, size_t count)
{
	if (!count)
		return;

	size_t oldpos = buffer->pos;
	size_t oldcapacity = buffer->capacity;

	buffer->pos += count;

	while (buffer->pos >= buffer->capacity)
		buffer->capacity *= 2;

	if (!buffer->data || buffer->capacity != oldcapacity)
		buffer->data = Z_Realloc(buffer->data, buffer->capacity, PU_STATIC, NULL);

	memcpy(&buffer->data[oldpos], mem, count);
}
