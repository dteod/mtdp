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

#ifndef MTDP_IMPL_PIPE_H
#define MTDP_IMPL_PIPE_H

#include <stdint.h>
#include <stddef.h>
#include "thread.h"

#include "impl/buffer.h"
#include "sem.h"

struct s_mtdp_pipe {
    mtx_t pool_mutex;
    mtx_t fifo_mutex;

    size_t total_buffers;
    mtdp_buffer_pool pool;
    mtdp_buffer_fifo fifo;

    mtdp_semaphore semaphore;
};

bool mtdp_pipe_init(mtdp_pipe*);
void mtdp_pipe_destroy(mtdp_pipe*);
void mtdp_pipe_clear(mtdp_pipe*);

mtdp_buffer  mtdp_pipe_get_empty_buffer(mtdp_pipe*);
bool         mtdp_pipe_push_buffer(mtdp_pipe*, mtdp_buffer);
mtdp_buffer  mtdp_pipe_get_full_buffer(mtdp_pipe*);
bool         mtdp_pipe_put_back(mtdp_pipe*, mtdp_buffer);


#if MTDP_PIPE_VECTOR_STATIC_SIZE
    typedef mtdp_pipe mtdp_pipe_vector[MTDP_PIPE_VECTOR_STATIC_SIZE];
#else
#   include <stdlib.h>
    typedef mtdp_pipe* mtdp_pipe_vector;
#endif

bool mtdp_pipe_vector_resize(mtdp_pipe_vector*, size_t);
void mtdp_pipe_vector_destroy(mtdp_pipe_vector*);

#endif
