// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by James R.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_threads.h
/// \brief Multithreading abstraction

#ifndef I_THREADS_H
#define I_THREADS_H

#include "doomtype.h"

typedef void (*I_thread_fn)(void *userdata);

typedef void * I_mutex;
typedef void * I_cond;

int       I_can_thread (void) FUNCWARNRV;
void      I_start_threads (void);
void      I_stop_threads  (void);

int       I_spawn_thread (const char *name, I_thread_fn, void *userdata) FUNCWARNRV;

/* check in your thread whether to return early */
int       I_thread_is_stopped (void);

void      I_lock_mutex      (I_mutex *);
void      I_unlock_mutex    (I_mutex);

void      I_hold_cond       (I_cond *, I_mutex);

void      I_wake_one_cond   (I_cond *);
void      I_wake_all_cond   (I_cond *);

#endif/*I_THREADS_H*/
