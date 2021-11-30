// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2003      by James Haley
// Copyright (C) 2003-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_queue.h
/// \brief General queue code

#ifndef M_QUEUE_H
#define M_QUEUE_H

typedef struct mqueueitem_s
{
	struct mqueueitem_s *next;
} mqueueitem_t;

typedef struct mqueue_s
{
	mqueueitem_t head;
	mqueueitem_t *tail;
	mqueueitem_t *rover;
} mqueue_t;

void M_QueueInit(mqueue_t *queue);
void M_QueueInsert(mqueueitem_t *item, mqueue_t *queue);
mqueueitem_t *M_QueueIterator(mqueue_t *queue);
void M_QueueResetIterator(mqueue_t *queue);
void M_QueueFree(mqueue_t *queue);

#endif

// EOF
