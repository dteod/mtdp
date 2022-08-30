/* Copyright (C) 2021-2022 Domenico Teodonio

mtdp is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

mtdp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "worker.h"
#include "impl/errno.h"

#include <string.h>

#ifdef __linux__
#   include <pthread.h>
#   include <sys/prctl.h>
#elif WIN32
#   include <string.h>
#   include <windows.h>
#   include <processthreadsapi.h>
#endif

#define thrd_check(X) do { int ret = (X); if(ret != thrd_success) { *mtdp_errno_ptr_mutable() = (ret  == thrd_nomem ? MTDP_NO_MEM : MTDP_THRD_ERROR); return false; } } while(false)
#define mtx_check(X)  do { int ret = (X); if(ret != thrd_success) { *mtdp_errno_ptr_mutable() = (ret  == thrd_nomem ? MTDP_NO_MEM : MTDP_MTX_ERROR ); return false; } } while(false)
#define cnd_check(X)  do { int ret = (X); if(ret != thrd_success) { *mtdp_errno_ptr_mutable() = (ret  == thrd_nomem ? MTDP_NO_MEM : MTDP_CND_ERROR ); return false; } } while(false)

#if defined(_WIN32)
#    define MTDP_WORKER_RETURN DWORD
#else
#    define MTDP_WORKER_RETURN int
#endif

static MTDP_WORKER_RETURN mtdp_worker_routine(void* data)
{
    mtdp_worker *worker = (mtdp_worker*) data;
    if(worker->name && strlen(worker->name)) {
#ifdef __linux__
        prctl(PR_SET_NAME, worker->name, 0, 0, 0);
#elif _WIN32
        size_t name_size = strlen(worker->name);
        if(name_size) {
            int sz = MultiByteToWideChar(CP_THREAD_ACP, MB_PRECOMPOSED, worker->name, -1, NULL, 0);
            WCHAR* utf16_name = (WCHAR*) malloc(sizeof(WCHAR)*sz);
            MultiByteToWideChar(CP_THREAD_ACP, MB_PRECOMPOSED, worker->name, -1, utf16_name, sz);
            SetThreadDescription(GetCurrentThread(), utf16_name);
            free(utf16_name);
        }
#endif
    }

    while(!worker->destroyed) {
        mtx_check(mtx_lock(&worker->mutex));
        while(!(worker->enabled || worker->destroyed)) {
            cnd_check(cnd_wait(&worker->cv, &worker->mutex));
        }
        mtx_check(mtx_unlock(&worker->mutex));
        if(worker->destroyed) {
            break;
        }
        worker->cb(worker->args);
    }
    thrd_exit(0);
}

bool mtdp_worker_init(mtdp_worker *worker)
{
    int out;

    worker->enabled = false;
    worker->destroyed = false;
    cnd_check(cnd_init(&worker->cv));
    if((out = mtx_init(&worker->mutex, mtx_plain)) != thrd_success) {
        /* No macro here, I have to destroy the cv. */
        cnd_destroy(&worker->cv);
        *mtdp_errno_ptr_mutable() = (out == thrd_nomem ? MTDP_NO_MEM : MTDP_MTX_ERROR);
        return false;
    }
    worker->name = NULL;
    worker->cb = NULL;
    worker->args = NULL;
    return true;
}

bool mtdp_worker_create_thread(mtdp_worker* worker)
{
    thrd_check(thrd_create(&worker->thread, mtdp_worker_routine, worker));
    return true;
}

bool mtdp_worker_enable(mtdp_worker* worker)
{
    mtx_check(mtx_lock(&worker->mutex));
    worker->enabled = true;
    /* This is terribly wrong: a deadlock is occurring here.
        The best we can do right now is to notify the user
        with an error code but a better error handling strategy
        shall be provided. */
    mtx_check(mtx_unlock(&worker->mutex));
    cnd_check(cnd_signal(&worker->cv));
    return true;
}

bool mtdp_worker_disable(mtdp_worker* worker)
{
    mtx_check(mtx_lock(&worker->mutex));
    worker->enabled = false;
    /* This is terribly wrong: a deadlock is occurring here.
        The best we can do right now is to notify the user
        with an error code but a better error handling strategy
        shall be provided. */
    mtx_check(mtx_unlock(&worker->mutex));
    cnd_check(cnd_signal(&worker->cv));
    return true;
}

bool mtdp_worker_destroy(mtdp_worker* worker)
{
    mtx_check(mtx_lock(&worker->mutex));
    worker->destroyed = true;
    mtx_check(mtx_unlock(&worker->mutex));
    cnd_check(cnd_signal(&worker->cv));
    return true;
}

bool mtdp_worker_join(mtdp_worker* worker)
{
    thrd_check(thrd_join(worker->thread, NULL));
    return true;
}
