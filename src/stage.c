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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mtdp/stage.h"
#include "impl/stage.h"
#include "impl/pipe.h"
#include "futex.h"
#include "memory.h"
#include "bell.h"

#if defined(__GNUC__) || defined(__clang__)
#   define likely(expr)    (__builtin_expect(!!(expr), 1))
#   define unlikely(expr)  (__builtin_expect(!!(expr), 0))
#else
#   define likely(expr) (expr)
#   define unlikely(expr) (expr)
#endif

bool mtdp_stage_vector_resize(mtdp_stage_vector* vector, size_t n)
{
#if MTDP_STAGE_VECTOR_STATIC_SIZE
    return n <= MTDP_STAGE_VECTOR_STATIC_SIZE ? (memset(vector, 0, sizeof(mtdp_stage)*MTDP_STAGE_VECTOR_STATIC_SIZE), true) : false;
#else
    return (*vector = (mtdp_stage_vector) calloc(n, sizeof(mtdp_stage))) != NULL;
#endif
}

void mtdp_stage_vector_destroy(mtdp_stage_vector* vector)
{
#if MTDP_STAGE_VECTOR_STATIC_SIZE
    (void) vector;
#else
    free(*vector);
#endif
}

bool mtdp_stage_impl_vector_resize(mtdp_stage_impl_vector* vector, size_t n)
{
#if MTDP_STAGE_IMPL_VECTOR_STATIC_SIZE
    return n <= MTDP_STAGE_IMPL_VECTOR_STATIC_SIZE ? memset(vector, 0, sizeof(mtdp_stage_impl)*MTDP_STAGE_IMPL_VECTOR_STATIC_SIZE) , true : false;
#else
    return (*vector = (mtdp_stage_impl_vector) malloc(sizeof(mtdp_stage_impl)*n)) != NULL;
#endif
}

void mtdp_stage_impl_vector_destroy(mtdp_stage_impl_vector* vector)
{
#if MTDP_STAGE_IMPL_VECTOR_STATIC_SIZE
    (void) vector;
#else
    free(*vector);
#endif
}

static int mtdp_stage_routine(void* data)
{
    mtdp_stage_impl* self = (mtdp_stage_impl*) data;

    if(self->context.ready_to_push) {
        if(likely(mtdp_pipe_push_buffer(self->output_pipe, self->context.output))) {
            mtdp_semaphore_release(&self->output_pipe->semaphore, 1);
            self->context.output = NULL;
            self->context.ready_to_push = false;
        } else {
            mtdp_set_done(&self->done);
            thrd_yield();
            return 0;
        }
    }
    if(self->context.ready_to_pull) {
        if(!mtdp_semaphore_try_acquire_for(&self->input_pipe->semaphore, MTDP_PIPELINE_CONSUMER_TIMEOUT_US)) {
            mtdp_set_done(&self->done);
            thrd_yield();
            return 0;
        }
        mtdp_unset_done(&self->done);
        self->context.input = mtdp_pipe_get_full_buffer(self->input_pipe);
        if(unlikely(!self->context.input)) {
            mtdp_semaphore_release(&self->input_pipe->semaphore, 1);
            thrd_yield();
            return 0;
        }
        self->context.ready_to_pull = false;
    }
    if(self->context.input) {
        if(!self->context.output) {
            self->context.output = mtdp_pipe_get_empty_buffer(self->output_pipe);
        }
        if(likely(self->context.output)) {
            if(unlikely(!self->initialized)) {
                if(self->user_data->init) {
                    self->user_data->init(&self->context);
                }
                self->initialized = true;
            }
            self->user_data->process(&self->context);
            if(self->context.ready_to_pull) {
                mtdp_pipe_put_back(self->input_pipe, self->context.input);
                self->context.input = NULL;
            }
        } else {
            thrd_yield();
        }
    } else {
        self->context.ready_to_pull = true;
    }
    return 0;
}

void mtdp_stage_create_thread(mtdp_stage_impl* self)
{
    self->worker.name = self->user_data->name;
    self->context.self = self->user_data->self;
    self->context.ready_to_pull = true;
    self->context.ready_to_push = false;
    self->context.input = self->context.output = NULL;
    self->done = 0;
    mtdp_worker_create_thread(&self->worker);
}

void mtdp_stage_destroy(mtdp_stage_impl* self)
{
    mtdp_worker_destroy(&self->worker);
    mtdp_set_done(&self->done);
}

void mtdp_stage_configure(mtdp_stage_impl* self, mtdp_pipe* input_pipe, mtdp_pipe* output_pipe, mtdp_stage* user_data)
{
    mtdp_worker_init(&self->worker);
    self->initialized = false;
    self->user_data = user_data;
    self->user_data->init = NULL;
    self->user_data->name = NULL;
    self->user_data->self = NULL;
    self->worker.cb = mtdp_stage_routine;
    self->worker.args = self;
    self->input_pipe  = input_pipe;
    self->output_pipe = output_pipe;
}

bool mtdp_stage_stop_requested(mtdp_stage_context* ctx) {
    mtdp_stage_impl* self = (mtdp_stage_impl*) ((char*)(ctx) + offsetof(mtdp_stage_impl, context));
    return self->worker.destroyed || !self->worker.enabled;
}