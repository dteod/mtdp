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
along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef MTDP_SEM_H
#define MTDP_SEM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if defined(_WIN32)
#   include <windows.h>
#elif defined(__unix__)
#   include <semaphore.h>
#endif

#if defined(_WIN32)
typedef HANDLE mtdp_semaphore;
#elif defined(__unix__)
typedef sem_t mtdp_semaphore;
#endif

#if defined(__unix__)
#   define mtdp_semaphore_init(sem)            (sem_init((sem), 0, 0) == 0)
#   define mtdp_semaphore_destroy(sem)         sem_destroy(sem)
#   define mtdp_semaphore_release(sem, amount) do { int32_t am = (amount); while(am--) { sem_post(sem); }; } while(0)
#   define mtdp_semaphore_acquire(self)        sem_wait(sem)
#   include <time.h>
#   if defined(__GNUC__) && !MTDP_STRICT_ISO_C
        /* Since the following is a define, the -Wpedantic ISO C diagnostic ignore shall be added to every usage of the function
#       if !defined(__clang__)
#           pragma GCC diagnostic push
#           pragma GCC diagnostic ignored "-Wpedantic"
#       else
#           pragma clang diagnostic push
#           pragma clang diagnostic ignored "-Wpedantic"
#       endif
        */
#       define mtdp_semaphore_try_acquire_for(sem, us) ({   \
    struct timespec ts;                                     \
    long seconds = (us)/1000000;                            \
    clock_gettime(CLOCK_REALTIME, &ts);                     \
    ts.tv_sec  += seconds;                                  \
    ts.tv_nsec += ((us) - seconds*1000000)*1000;            \
    ts.tv_sec  += ts.tv_nsec/1000000000;                    \
    ts.tv_nsec %= 1000000000;                               \
    sem_timedwait((sem), &ts) == 0;                         \
        })
        /* Also remember to pop the diagnostic!
#       if !defined(__clang__)
#           pragma GCC diagnostic pop
#       else
#           pragma clang diagnostic pop
#       endif
        */
#   else
        inline bool mtdp_semaphore_try_acquire_for(mtdp_semaphore* self, unsigned microseconds) {
    struct timespec ts;
    unsigned seconds = microseconds/1000000;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec  += seconds;
    ts.tv_nsec += (microseconds - seconds*1000000)*1000;
    ts.tv_sec  += ts.tv_nsec/1000000000;
    ts.tv_nsec %= 1000000000;
    return sem_timedwait(self, &ts) == 0;
        }
#   endif
#elif defined(_WIN32)
#   define mtdp_semaphore_init(p_sem)               (*(p_sem) = CreateSemaphoreA(NULL, 0, INT32_MAX, NULL))
#   define mtdp_semaphore_destroy(p_sem)            CloseHandle(*(p_sem))
#   define mtdp_semaphore_release(p_sem, update)    ReleaseSemaphore(*(p_sem), update, NULL)
#   define mtdp_semaphore_acquire(p_sem)            WaitForSingleObject(*(p_sem), INFINITE)
#   define mtdp_semaphore_try_acquire_for(p_sem, us)  (WaitForSingleObject(*(p_sem), (us)/1000) == WAIT_OBJECT_0)
#endif

#endif
