// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by James R.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_threads.h
/// \brief Multithreading abstraction

#ifdef HAVE_THREADS

#ifndef I_THREADS_H
#define I_THREADS_H

typedef void (*I_thread_fn)(void *userdata);
typedef void * I_thread_handle;

#ifdef HAVE_SDL
#include <SDL.h>
#define ATOMIC_TYPE SDL_atomic_t
#else
#define ATOMIC_TYPE UINT32
#endif

typedef ATOMIC_TYPE I_Atomicval_t;
typedef I_Atomicval_t * I_Atomicptr_t;

typedef void * I_mutex;
typedef void * I_cond;

void            I_start_threads (void);
void            I_stop_threads  (void);

I_thread_handle I_spawn_thread (const char *name, I_thread_fn, void *userdata);

/* check in your thread whether to return early */
int             I_thread_is_stopped (void);

void            I_lock_mutex      (I_mutex *);
void            I_unlock_mutex    (I_mutex);

void            I_hold_cond       (I_cond *, I_mutex);

void            I_wake_one_cond   (I_cond *);
void            I_wake_all_cond   (I_cond *);

INT32           I_atomic_load     (I_Atomicptr_t atomic);
INT32           I_atomic_exchange (I_Atomicptr_t atomic, INT32 val);

#endif/*I_THREADS_H*/
#endif/*HAVE_THREADS*/
