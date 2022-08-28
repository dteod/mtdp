#ifndef MTDP_THREAD_H
#define MTDP_THREAD_H

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32)
#   include <windows.h>

#define thread_local _Thread_local

typedef HANDLE thrd_t;

typedef int(*thrd_start_t)(void*);

enum {
  thrd_success,
  thrd_busy,
  thrd_error,
  thrd_nomem,
  thrd_timedout
};

typedef _Atomic(uint8_t) once_flag;
#define ONCE_FLAG_INIT 0

int thrd_create(thrd_t*, thrd_start_t, void*);
int thrd_join(thrd_t, int*);
void thrd_yield();
noreturn void thrd_exit(int);

enum {
    mtx_plain,
    mtx_recursive,
    mtx_timed
};

typedef HANDLE mtx_t;
int mtx_init(mtx_t*, int);
int mtx_lock(mtx_t*);
int mtx_unlock(mtx_t*);

typedef HANDLE cnd_t;

int cnd_init(cnd_t*);
int cnd_wait (cnd_t*, mtx_t*);
int cnd_signal(cnd_t*);
int cnd_destroy(cnd_t*);

#elif __unix__
#   include <threads.h>
#else
#endif

#endif