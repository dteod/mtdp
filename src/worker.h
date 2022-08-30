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

#ifndef MTDP_WORKER_H
#define MTDP_WORKER_H

#include <stdbool.h>
#include "atomic.h"
#include "thread.h"

typedef struct {
    thrd_t thread;
    cnd_t cv;
    mtx_t mutex;
    bool enabled, destroyed;

    const char* name;
    thrd_start_t cb;
    void* args;
} mtdp_worker;

bool mtdp_worker_init(mtdp_worker *worker);
bool mtdp_worker_create_thread(mtdp_worker *worker);
bool mtdp_worker_enable(mtdp_worker *worker);
bool mtdp_worker_disable(mtdp_worker *worker);
bool mtdp_worker_destroy(mtdp_worker *worker);
bool mtdp_worker_join(mtdp_worker *worker);

#endif
