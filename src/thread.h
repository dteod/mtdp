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


#ifndef MTDP_THREAD_H
#define MTDP_THREAD_H

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32)
#   include <windows.h>

typedef HANDLE thrd_t;

typedef int(*thrd_start_t)(void*);

enum {
  thrd_success,
  thrd_busy,
  thrd_error,
  thrd_nomem,
  thrd_timedout
};

#define once_flag atomic_flag;

#define ONCE_FLAG_INIT { 0 }
#define call_once(onceflag, fun) do { ( if(atomic_flag_test_and_set(onceflag)) (fun)(); } while(0)
#define thrd_create(t, f, data) (*(t) = CreateThread(NULL, 0, f, data, 0, NULL), ((t) ? thrd_success : ((GetLastError() == ERROR_NOT_ENOUGH_MEMORY) ? thrd_nomem : thrd_error)))
inline int thrd_join(thrd_t t, int* ret)
{
    int out = thrd_success;
    WaitForSingleObject(t, INFINITE);
    if(GetLastError() != ERROR_SUCCESS) {
        out = thrd_error;
    }
    if(ret) {
        DWORD uret;
        GetExitCodeThread(t, &uret);
        *ret = (int)uret;
        if(GetLastError() != ERROR_SUCCESS) {
            out = thrd_error;
        }
    }
    return out;
}
#define thrd_yield() SwitchToThread()
#define thrd_exit(ret) ExitThread(ret)

enum {
    mtx_plain,
    mtx_recursive,
    mtx_timed
};

typedef CRITICAL_SECTION mtx_t;

#define mtx_init(m, type) (InitializeCriticalSection(m), thrd_success)
#define mtx_lock(m) (EnterCriticalSection(m), thrd_success)
#define mtx_trylock(m) (TryEnterCriticalSection(m), thrd_success)
#define mtx_unlock(m) (LeaveCriticalSection(m), thrd_success)
#define mtx_destroy(m) (DeleteCriticalSection(m), thrd_success)

typedef CONDITION_VARIABLE cnd_t;

#define cnd_init(c) (InitializeConditionVariable(c), thrd_success)
#define cnd_wait(c, m) (SleepConditionVariableCS(c, m, INFINITE) != 0 ? thrd_success : thrd_error)
#define cnd_signal(c) (WakeConditionVariable(c), thrd_success)
#define cnd_destroy(c)

#elif __unix__
#   include <threads.h>
#else
#   error threads not implemented on this platform
#endif

#endif