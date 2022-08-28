#include "thread.h"

#if defined(_WIN32)

#   include <synchapi.h>
#   include <processthreadapi.h>
#   include <errhandlingapi.h>

int thrd_create(thrd_t* t, thrd_start_t f, void* data)
{
    *t = CreateThread(
        NULL, 0, f, data, 0, NULL
    );
    if(!*t) {
        return GetLatestError() == ERROR_NOT_ENOUGH_MEMORY ? thrd_nomem : thrd_error;
    }
    return thrd_success;
}

int thrd_join(thrd_t t, int* ret)
{
    int out = thrd_success;
    WaitForSingleObject(t, INFINITE);
    if(GetLastError() != ERROR_SUCCESS) {
        out = thrd_error;
    }
    if(ret) {
        GetExitCodeThread(t, ret);
        if(GetLastError() != ERROR_SUCCESS) {
            out = thrd_error;
        }
    }
    return out;
}

void thrd_yield()
{
    SwitchToThread();
}

noreturn void thrd_exit(int ret)
{
    ExitThread(ret);
}

enum {
    mtx_plain,
    mtx_recursive,
    mtx_timed
};

typedef CRITICAL_SECTION mtx_t;
int mtx_init(mtx_t* m, int type)
{
    if(type != mtx_plain) {
        /* Only using plain mutexes in mtdp */
        return thrd_error;
    }
    InitializeCriticalSection(m);
    return thrd_success;
}

int mtx_lock(mtx_t* m)
{
    EnterCriticalSection(m);
    return thrd_success;
}

int mtx_unlock(mtx_t* m)
{
    LeaveCriticalSection(m);
    return thrd_success;
}

void mtd_destroy(mtx_t* m)
{
    DeleteCriticalSection(m);
    return thrd_success;
}

typedef HANDLE cnd_t;

int cnd_init(cnd_t* c)
{
    InitializeConditionVariable(c);
}

int cnd_wait(cnd_t* c, mtx_t* m)
{
    return SleepConditionVariableCS(c, m, INFINITE) != 0 : thrd_success : thrd_error;
}

int cnd_signal(cnd_t*)
{
    WakeConditionVariable(c);
    return thrd_success;
}

int cnd_destroy(cnd_t* c)
{

}

#endif