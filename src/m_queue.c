// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2003      by James Haley
// Copyright (C) 2003-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_queue.c
/// \brief General queue code

#include <stdlib.h>

#include "z_zone.h"
#include "m_queue.h"
#include "m_misc.h"

//
// M_QueueInit
//
// Sets up a queue. Can be called again to reset a used queue
// structure.
//
void M_QueueInit(mqueue_t *queue)
{
	queue->head.next = NULL;
	queue->tail = &(queue->head);
	queue->rover = &(queue->head);
}

//
// M_QueueInsert
//
// Inserts the given item into the queue.
//
void M_QueueInsert(mqueueitem_t *item, mqueue_t *queue)
{
	// link in at the tail (this works even for the first node!)
	queue->tail = queue->tail->next = item;
}

//
// M_QueueIterator
//
// Returns the next item in the queue each time it is called,
// or NULL once the end is reached. The iterator can be reset
// using M_QueueResetIterator.
//
mqueueitem_t *M_QueueIterator(mqueue_t *queue)
{
	if (queue->rover == NULL)
		return NULL;

	return (queue->rover = queue->rover->next);
}

//
// M_QueueResetIterator
//
// Returns the queue iterator to the beginning.
//
void M_QueueResetIterator(mqueue_t *queue)
{
	queue->rover = &(queue->head);
}

//
// M_QueueFree
//
// Frees all the elements in the queue
//
void M_QueueFree(mqueue_t *queue)
{
	mqueueitem_t *rover = queue->head.next;

	while (rover)
	{
		mqueueitem_t *next = rover->next;
		free(rover);

		rover = next;
	}

	M_QueueInit(queue);
}

// EOF
