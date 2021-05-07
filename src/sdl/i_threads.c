// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by James R.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_threads.c
/// \brief Multithreading abstraction

#include "../doomdef.h"
#include "../i_threads.h"

#include <SDL.h>

typedef void * (*Create_fn)(void);

struct Link;
struct Thread;

typedef struct Link   * Link;
typedef struct Thread * Thread;

struct Link
{
	void * data;
	Link   next;
	Link   prev;
};

struct Thread
{
	I_thread_fn   entry;
	void        * userdata;

	SDL_Thread  * thread;
};

static Link    i_thread_pool;
static Link    i_mutex_pool;
static Link    i_cond_pool;

static I_mutex        i_thread_pool_mutex;
static I_mutex        i_mutex_pool_mutex;
static I_mutex        i_cond_pool_mutex;

static SDL_atomic_t   i_threads_running = {1};

static Link
Insert_link (
		Link * head,
		Link   link
){
	link->prev = NULL;
	link->next = (*head);
	if ((*head))
		(*head)->prev = link;
	(*head)    = link;
	return link;
}

static void
Free_link (
		Link * head,
		Link   link
){
	if (link->prev)
		link->prev->next = link->next;
	else
		(*head) = link->next;

	if (link->next)
		link->next->prev = link->prev;

	free(link->data);
	free(link);
}

static Link
New_link (void *data)
{
	Link link;

	link = malloc(sizeof *link);

	if (! link)
		abort();

	link->data = data;

	return link;
}

static void *
Identity (
		Link      *  pool_anchor,
		I_mutex      pool_mutex,

		void      ** anchor,

		Create_fn    create_fn
){
	void * id;

	id = SDL_AtomicGetPtr(anchor);

	if (! id)
	{
		I_lock_mutex(&pool_mutex);
		{
			id = SDL_AtomicGetPtr(anchor);

			if (! id)
			{
				id = (*create_fn)();

				if (! id)
					abort();

				Insert_link(pool_anchor, New_link(id));

				SDL_AtomicSetPtr(anchor, id);
			}
		}
		I_unlock_mutex(pool_mutex);
	}

	return id;
}

static int
Worker (
		Link link
){
	Thread th;

	th = link->data;

	(*th->entry)(th->userdata);

	if (SDL_AtomicGet(&i_threads_running))
	{
		I_lock_mutex(&i_thread_pool_mutex);
		{
			if (SDL_AtomicGet(&i_threads_running))
			{
				SDL_DetachThread(th->thread);
				Free_link(&i_thread_pool, link);
			}
		}
		I_unlock_mutex(i_thread_pool_mutex);
	}

	return 0;
}

void
I_spawn_thread (
		const char  * name,
		I_thread_fn   entry,
		void        * userdata
){
	Link   link;
	Thread th;

	th = malloc(sizeof *th);

	if (! th)
		abort();/* this is pretty GNU of me */

	th->entry    = entry;
	th->userdata = userdata;

	I_lock_mutex(&i_thread_pool_mutex);
	{
		link = Insert_link(&i_thread_pool, New_link(th));

		if (SDL_AtomicGet(&i_threads_running))
		{
			th->thread = SDL_CreateThread(
					(SDL_ThreadFunction)Worker,
					name,
					link
			);

			if (! th->thread)
				abort();
		}
	}
	I_unlock_mutex(i_thread_pool_mutex);
}

int
I_thread_is_stopped (void)
{
	return ( ! SDL_AtomicGet(&i_threads_running) );
}

void
I_start_threads (void)
{
	i_thread_pool_mutex = SDL_CreateMutex();
	i_mutex_pool_mutex  = SDL_CreateMutex();
	i_cond_pool_mutex   = SDL_CreateMutex();

	if (!(
				i_thread_pool_mutex &&
				i_mutex_pool_mutex  &&
				i_cond_pool_mutex
	)){
		abort();
	}
}

void
I_stop_threads (void)
{
	Link        link;
	Link        next;

	Thread      th;
	SDL_mutex * mutex;
	SDL_cond  * cond;

	if (i_threads_running.value)
	{
		/* rely on the good will of thread-san */
		SDL_AtomicSet(&i_threads_running, 0);

		I_lock_mutex(&i_thread_pool_mutex);
		{
			for (
					link = i_thread_pool;
					link;
					link = next
			){
				next = link->next;
				th   = link->data;

				SDL_WaitThread(th->thread, NULL);

				free(th);
				free(link);
			}
		}
		I_unlock_mutex(i_thread_pool_mutex);

		for (
				link = i_mutex_pool;
				link;
				link = next
		){
			next  = link->next;
			mutex = link->data;

			SDL_DestroyMutex(mutex);

			free(link);
		}

		for (
				link = i_cond_pool;
				link;
				link = next
		){
			next = link->next;
			cond = link->data;

			SDL_DestroyCond(cond);

			free(link);
		}

		SDL_DestroyMutex(i_thread_pool_mutex);
		SDL_DestroyMutex(i_mutex_pool_mutex);
		SDL_DestroyMutex(i_cond_pool_mutex);
	}
}

void
I_lock_mutex (
		I_mutex * anchor
){
	SDL_mutex * mutex;

	mutex = Identity(
			&i_mutex_pool,
			i_mutex_pool_mutex,
			anchor,
			(Create_fn)SDL_CreateMutex
	);

	if (SDL_LockMutex(mutex) == -1)
		abort();
}

void
I_unlock_mutex (
		I_mutex id
){
	if (SDL_UnlockMutex(id) == -1)
		abort();
}

void
I_hold_cond (
		I_cond  * cond_anchor,
		I_mutex   mutex_id
){
	SDL_cond * cond;

	cond = Identity(
			&i_cond_pool,
			i_cond_pool_mutex,
			cond_anchor,
			(Create_fn)SDL_CreateCond
	);

	if (SDL_CondWait(cond, mutex_id) == -1)
		abort();
}

void
I_wake_one_cond (
		I_cond * anchor
){
	SDL_cond * cond;

	cond = Identity(
			&i_cond_pool,
			i_cond_pool_mutex,
			anchor,
			(Create_fn)SDL_CreateCond
	);

	if (SDL_CondSignal(cond) == -1)
		abort();
}

void
I_wake_all_cond (
		I_cond * anchor
){
	SDL_cond * cond;

	cond = Identity(
			&i_cond_pool,
			i_cond_pool_mutex,
			anchor,
			(Create_fn)SDL_CreateCond
	);

	if (SDL_CondBroadcast(cond) == -1)
		abort();
}
