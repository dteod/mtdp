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

#include "sem.h"

#include <stdlib.h>
#include <errno.h>

#if defined(_WIN32)
#   include <synchapi.h>
    void mtdp_semaphore_create(mtdp_semaphore* sem)
    {
        if(sem) {
            sem->semaphore = CreateSemaphoreA( 
                NULL,           // default security attributes
                0,              // initial count
                MAX_SEM_COUNT,  // maximum count
                NULL            // unnamed semaphore
            );
        }
    }

    void mtdp_semaphore_destroy(mtdp_semaphore* self)
    {
        if(self) {
            CloseHandle(self->semaphore);
            free(self);
        }
    }

    void mtdp_semaphore_release(mtdp_semaphore* self, ptrdiff_t update)
    {
        if(self) {
            ReleaseSemaphore(self->semaphore, update, NULL);
        }
    }

    void mtdp_semaphore_acquire(mtdp_semaphore* self)
    {
        if(self) {
            WaitForSingleObject(self->semaphore, INFINITE);
        }
    }

    bool mtdp_semaphore_try_acquire_for(mtdp_semaphore* self, unsigned microseconds)
    {
        if(self) {
            return WaitForSingleObject(self->semaphore, microseconds/1000) == WAIT_OBJECT_0;
        }
    }
#elif defined(__unix__)
#   include <time.h>
    void mtdp_semaphore_init(mtdp_semaphore* sem)
    {
        if(sem) {
            sem_init(&sem->semaphore, 0, 0);
        }
    }

    void mtdp_semaphore_destroy(mtdp_semaphore* sem)
    {
        if(sem) {
            sem_destroy(&sem->semaphore);
            free(sem);
        }
    }

    void mtdp_semaphore_release(mtdp_semaphore* self, ptrdiff_t update)
    {
        if(self) {
            for(; update--; ) {
                sem_post(&self->semaphore);
            }
        }
    }

    void mtdp_semaphore_acquire(mtdp_semaphore* self)
    {
        if(self) {
            sem_wait(&self->semaphore);
        }
    }

    static const uint32_t MILLION = 1000000;
    static const uint32_t BILLION = 1000*MILLION;

    bool mtdp_semaphore_try_acquire_for(mtdp_semaphore* self, unsigned microseconds)
    {
        struct timespec ts;
        unsigned seconds = microseconds/MILLION;

        if(self) {
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec  += seconds;
            ts.tv_nsec += (microseconds - seconds*MILLION)*1000;
            ts.tv_sec += ts.tv_nsec/BILLION;
            ts.tv_nsec %= BILLION;
            return sem_timedwait(&self->semaphore, &ts) == 0;
        }
        return false;
    }
#else
#   error mtdp_semaphore not implemented on this platform
#endif
