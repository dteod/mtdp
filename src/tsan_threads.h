/* Copyright (C) 2021-2022 Domenico Teodonio

mtdp is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

mtdp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* ThreadSanitizer support shim.

ThreadSanitizer intercepts the pthread API but not the C11 <threads.h> one:
threads started with thrd_create are unknown to its runtime (the program
crashes as soon as they run instrumented code) and locks taken with mtx_lock
are invisible to it (every protected access is reported as a race).

This header redirects the C11 threads API to the pthread one. It is force
included in every translation unit (-include) when configuring with
-DMTDP_ENABLE_TSAN=true, and it is never used otherwise. It relies on glibc
laying out thrd_t, mtx_t, cnd_t and once_flag as their pthread counterparts,
which is what glibc itself does to implement <threads.h>. */

#ifndef MTDP_TSAN_THREADS_H
#define MTDP_TSAN_THREADS_H

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <threads.h>

#if !defined(__GLIBC__)
#  error the ThreadSanitizer build of mtdp is only supported with glibc
#endif

_Static_assert(sizeof(thrd_t) == sizeof(pthread_t), "thrd_t is not a pthread_t");
_Static_assert(sizeof(mtx_t) >= sizeof(pthread_mutex_t), "mtx_t cannot hold a pthread_mutex_t");
_Static_assert(sizeof(cnd_t) >= sizeof(pthread_cond_t), "cnd_t cannot hold a pthread_cond_t");
_Static_assert(sizeof(once_flag) == sizeof(pthread_once_t), "once_flag is not a pthread_once_t");

static inline int
mtdp_tsan_thrd_err(int err)
{
    switch(err) {
        case 0:         return thrd_success;
        case ENOMEM:    return thrd_nomem;
        case EBUSY:     return thrd_busy;
        case ETIMEDOUT: return thrd_timedout;
        default:        return thrd_error;
    }
}

typedef struct {
    thrd_start_t func;
    void*        arg;
} mtdp_tsan_thrd_start;

static void*
mtdp_tsan_thrd_trampoline(void* data)
{
    mtdp_tsan_thrd_start start = *(mtdp_tsan_thrd_start*)data;
    free(data);
    return (void*)(intptr_t)start.func(start.arg);
}

static inline int
mtdp_tsan_thrd_create(thrd_t* thr, thrd_start_t func, void* arg)
{
    mtdp_tsan_thrd_start* start = (mtdp_tsan_thrd_start*)malloc(sizeof(*start));
    if(!start) {
        return thrd_nomem;
    }
    start->func = func;
    start->arg  = arg;
    int err     = pthread_create((pthread_t*)thr, NULL, mtdp_tsan_thrd_trampoline, start);
    if(err) {
        free(start);
        return err == EAGAIN ? thrd_nomem : mtdp_tsan_thrd_err(err);
    }
    return thrd_success;
}

static inline int
mtdp_tsan_thrd_join(thrd_t thr, int* res)
{
    void* ret = NULL;
    int   err = pthread_join((pthread_t)thr, &ret);
    if(!err && res) {
        *res = (int)(intptr_t)ret;
    }
    return mtdp_tsan_thrd_err(err);
}

static inline int
mtdp_tsan_mtx_init(mtx_t* mtx, int type)
{
    pthread_mutexattr_t attr;
    int                 err = pthread_mutexattr_init(&attr);
    if(err) {
        return mtdp_tsan_thrd_err(err);
    }
    pthread_mutexattr_settype(&attr, (type & mtx_recursive) ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL);
    err = pthread_mutex_init((pthread_mutex_t*)mtx, &attr);
    pthread_mutexattr_destroy(&attr);
    return mtdp_tsan_thrd_err(err);
}

#define thrd_create(thr, func, arg) mtdp_tsan_thrd_create((thr), (func), (arg))
#define thrd_join(thr, res)         mtdp_tsan_thrd_join((thr), (res))
#define thrd_detach(thr)            mtdp_tsan_thrd_err(pthread_detach((pthread_t)(thr)))
#define thrd_exit(res)              pthread_exit((void*)(intptr_t)(res))

#define mtx_init(mtx, type) mtdp_tsan_mtx_init((mtx), (type))
#define mtx_lock(mtx)       mtdp_tsan_thrd_err(pthread_mutex_lock((pthread_mutex_t*)(mtx)))
#define mtx_trylock(mtx)    mtdp_tsan_thrd_err(pthread_mutex_trylock((pthread_mutex_t*)(mtx)))
#define mtx_unlock(mtx)     mtdp_tsan_thrd_err(pthread_mutex_unlock((pthread_mutex_t*)(mtx)))
#define mtx_destroy(mtx)    ((void)pthread_mutex_destroy((pthread_mutex_t*)(mtx)))

#define cnd_init(cnd)      mtdp_tsan_thrd_err(pthread_cond_init((pthread_cond_t*)(cnd), NULL))
#define cnd_signal(cnd)    mtdp_tsan_thrd_err(pthread_cond_signal((pthread_cond_t*)(cnd)))
#define cnd_broadcast(cnd) mtdp_tsan_thrd_err(pthread_cond_broadcast((pthread_cond_t*)(cnd)))
#define cnd_wait(cnd, mtx) mtdp_tsan_thrd_err(pthread_cond_wait((pthread_cond_t*)(cnd), (pthread_mutex_t*)(mtx)))
#define cnd_destroy(cnd)   ((void)pthread_cond_destroy((pthread_cond_t*)(cnd)))

#define call_once(flag, func) ((void)pthread_once((pthread_once_t*)(flag), (func)))

#endif
