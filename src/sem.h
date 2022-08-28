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

#ifndef MTDP_SEM_H
#define MTDP_SEM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if defined(_WIN32)
#   include <windows.h>
#   include <winsock.h> /* For struct timeval */
#elif defined(__unix__)
#   include <semaphore.h>
#endif

typedef struct
{
#if defined(_WIN32)
    HANDLE semaphore;
#elif defined(__unix__)
    sem_t  semaphore;
#endif
} mtdp_semaphore;

void mtdp_semaphore_init(mtdp_semaphore*);
void mtdp_semaphore_destroy(mtdp_semaphore*);

void mtdp_semaphore_release(mtdp_semaphore* self, ptrdiff_t update);
void mtdp_semaphore_acquire(mtdp_semaphore* self);
bool mtdp_semaphore_try_acquire(mtdp_semaphore* self);
bool mtdp_semaphore_try_acquire_for(mtdp_semaphore* self, unsigned microseconds);
bool mtdp_semaphore_try_acquire_until(mtdp_semaphore *restrict self, const struct timeval *restrict ts);

#endif
