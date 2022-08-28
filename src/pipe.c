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

#include <assert.h>
#include "mtdp/pipe.h"
#include "impl/pipe.h"
#include "impl/errno.h"
#include "memory.h"

#include <threads.h>

static inline void mtdp_lock2(mtx_t *restrict m1, mtx_t *restrict m2)
{
    while(true) {
        mtx_lock(m1);
        if(mtx_trylock(m2) == thrd_success) {
            break;
        }
        mtx_unlock(m1);
        thrd_yield();
        mtx_lock(m2);
        if(mtx_trylock(m1) == thrd_success) {
            break;
        }
        mtx_unlock(m2);
    }
}

mtdp_pipe* mtdp_pipe_next(mtdp_pipe* pipe)
{
    return pipe + 1;
}

/* Always called in an assertion: effectively removed in release builds */
inline static bool mtdp_pipe_check_invariants(mtdp_pipe* pipe)
{
    bool out;
    size_t total;

    mtdp_lock2(&pipe->pool_mutex, &pipe->fifo_mutex);
    total = pipe->fifo.size + pipe->pool.size;
    /* 
        The following expression also accounts for input/output stages
        holding memory during the pipe operations.
    */
    out = pipe->total_buffers == total 
        || pipe->total_buffers == total+1
        || pipe->total_buffers == total+2;
    mtx_unlock(&pipe->pool_mutex);
    mtx_unlock(&pipe->fifo_mutex);

    return out;
}

mtdp_buffer* mtdp_pipe_resize(mtdp_pipe* self, size_t n_buffers)
{
    mtdp_buffer* ret = self ? self->pool.buffers : NULL;
    ptrdiff_t delta;
    size_t total, empty, nonempty = 0;

    mtdp_errno_location =  self ? MTDP_OK : MTDP_BAD_PTR;
    if(self) {
        mtdp_lock2(&self->pool_mutex, &self->fifo_mutex);
        empty = mtdp_buffer_pool_size(&self->pool);
        nonempty += mtdp_buffer_fifo_size(&self->fifo);
        total = nonempty + empty;
        delta = (ptrdiff_t)( (ptrdiff_t) n_buffers - (ptrdiff_t) total );
        if(delta > 0) {
            if(!mtdp_buffer_pool_resize(&self->pool, empty + delta)) {
                mtdp_errno_location = MTDP_NO_MEM;
                ret = NULL;
            } else {
                ret = self->pool.buffers;
            }
        } else if(delta < 0) {
            if((size_t) -delta <= empty) {
                /*
                    Safe since it's a shrink,
                    but there would be a memory leak if the buffers
                    were unique owners of memory.
                    The user has been warned in the documentation.
                */
                mtdp_buffer_pool_resize(&self->pool, empty + delta);
                if((size_t) -delta <= total) {
                    /* Removing oldest entries from the ready deque. */
                    for(int i = total + delta; i--; ) {
                        /* We are shrinking here so no check is required. */
                        mtdp_buffer_fifo_pop_front(&self->fifo, NULL);
                    }
                }
            }
            ret = self->pool.buffers;
        }
        self->total_buffers = n_buffers;
        mtx_unlock(&self->pool_mutex);
        mtx_unlock(&self->fifo_mutex);
        assert(mtdp_pipe_check_invariants(self));
    }
    return ret;
}

mtdp_buffer* mtdp_pipe_buffers(mtdp_pipe* self)
{
    mtdp_errno_location =  self ? MTDP_OK : MTDP_BAD_PTR;
    return self ? self->pool.buffers : NULL;
}

void mtdp_pipe_clear(mtdp_pipe* self)
{
    mtdp_buffer tmp;

    if(self) {
        assert(mtdp_pipe_check_invariants(self));

        mtdp_lock2(&self->pool_mutex, &self->fifo_mutex);
        for(size_t i = mtdp_buffer_fifo_size(&self->fifo); i--; ) {
            mtdp_buffer_fifo_pop_front(&self->fifo, &tmp);
            mtdp_buffer_pool_push_back(&self->pool, tmp);
        }
        mtx_unlock(&self->pool_mutex);
        mtx_unlock(&self->fifo_mutex);
        assert(mtdp_pipe_check_invariants(self));
    }
}

bool mtdp_pipe_init(mtdp_pipe* pipe)
{
    mtdp_buffer_pool_init(&pipe->pool);
    if(mtx_init(&pipe->pool_mutex, mtx_plain) != thrd_success) {
        return false;
    }
    if(mtx_init(&pipe->fifo_mutex, mtx_plain) != thrd_success) {
        mtx_destroy(&pipe->pool_mutex);
        return false;
    }
    if(!mtdp_buffer_fifo_init(&pipe->fifo)) {
        mtx_destroy(&pipe->pool_mutex);
        mtx_destroy(&pipe->fifo_mutex);
        return false;
    }
    mtdp_semaphore_init(&pipe->semaphore);
    return true;
}

void mtdp_pipe_destroy(mtdp_pipe* pipe)
{
    mtdp_buffer_pool_destroy(&pipe->pool);
    mtdp_buffer_fifo_destroy(&pipe->fifo);
}

mtdp_buffer mtdp_pipe_get_empty_buffer(mtdp_pipe* self)
{
    mtdp_buffer out;

    assert(mtdp_pipe_check_invariants(self));
    mtx_lock(&self->pool_mutex);
    out = mtdp_buffer_pool_pop_back(&self->pool);
    mtx_unlock(&self->pool_mutex);
    
    assert(mtdp_pipe_check_invariants(self));
    return out;
}

bool mtdp_pipe_push_buffer(mtdp_pipe* self, mtdp_buffer buf)
{
    bool out;
    assert(mtdp_pipe_check_invariants(self));

    mtx_lock(&self->fifo_mutex);
    out = mtdp_buffer_fifo_push_back(&self->fifo, buf);
    mtx_unlock(&self->fifo_mutex);

    assert(mtdp_pipe_check_invariants(self));
    return out;
}

mtdp_buffer mtdp_pipe_get_full_buffer(mtdp_pipe* self)
{
    mtdp_buffer out = NULL;
    assert(mtdp_pipe_check_invariants(self));

    mtx_lock(&self->fifo_mutex);
    mtdp_buffer_fifo_pop_front(&self->fifo, &out);
    mtx_unlock(&self->fifo_mutex);

    assert(mtdp_pipe_check_invariants(self));
    return out;
}


bool mtdp_pipe_put_back(mtdp_pipe* self, mtdp_buffer buf)
{
    bool out;
    assert(mtdp_pipe_check_invariants(self));

    mtx_lock(&self->pool_mutex);
    out = mtdp_buffer_pool_push_back(&self->pool, buf);
    mtx_unlock(&self->pool_mutex);

    assert(mtdp_pipe_check_invariants(self));
    return out;
}

bool mtdp_pipe_vector_resize(mtdp_pipe_vector* vector, size_t n)
{
#if MTDP_PIPE_VECTOR_STATIC_SIZE
    (void) vector;
    return n <= MTDP_PIPE_VECTOR_STATIC_SIZE;
#else
    return (*vector = (mtdp_pipe_vector) malloc(sizeof(mtdp_pipe)*n)) != NULL;
#endif
}

void mtdp_pipe_vector_destroy(mtdp_pipe_vector* vector)
{
#if MTDP_PIPE_VECTOR_STATIC_SIZE
    (void) vector;

#else
    free(*vector);
#endif
}