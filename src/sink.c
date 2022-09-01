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
along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

#include "mtdp/sink.h"
#include "impl/sink.h"
#include "impl/pipe.h"
#include "bell.h"

#include <stddef.h>
#include "thread.h"

#if defined(__GNUC__) || defined(__clang__)
#   define likely(expr)    (__builtin_expect(!!(expr), 1))
#   define unlikely(expr)  (__builtin_expect(!!(expr), 0))
#else
#   define likely(expr) (expr)
#   define unlikely(expr) (expr)
#endif

static int mtdp_sink_routine(void* data)
{
    mtdp_sink_impl* self = (mtdp_sink_impl*) data;

    if(self->context.ready_to_pull) {
        if(self->context.input) {
            if(mtdp_pipe_put_back(self->input_pipe, self->context.input)) {
                self->context.input = NULL;
            } else {
                mtdp_set_done(&self->done);
                thrd_yield();
                return 0;
            }
        }
#if defined(__GNUC__) && !MTDP_STRICT_ISO_C
#    if !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#    else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
#    endif
#endif
        if(!mtdp_semaphore_try_acquire_for(&self->input_pipe->semaphore, MTDP_PIPELINE_CONSUMER_TIMEOUT_US)) {
#if defined(__GNUC__) && !MTDP_STRICT_ISO_C
#    if !defined(__clang__)
#pragma GCC diagnostic pop
#    else
#pragma clang diagnostic pop
#    endif
#endif
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
    if(likely(self->context.input)) {
        if(unlikely(!self->initialized)) {
            if(self->user_data.init) {
                self->user_data.init(&self->context);
            }
            self->initialized = true;
        }
        self->user_data.process(&self->context);
    } else {
        self->context.ready_to_pull = true;
        thrd_yield();
    }
    return 0;
}

void mtdp_sink_create_thread(mtdp_sink_impl* self)
{
    self->worker.name = self->user_data.name;
    self->context.self = self->user_data.self;
    self->context.ready_to_pull = true;
    self->context.input = NULL;
    self->done = 0;
    mtdp_worker_create_thread(&self->worker);
}

void mtdp_sink_destroy(mtdp_sink_impl* self)
{
    mtdp_worker_destroy(&self->worker);
    mtdp_set_done(&self->done);
}

void mtdp_sink_configure(mtdp_sink_impl* self, mtdp_pipe* input_pipe)
{
    self->initialized = false;
    mtdp_worker_init(&self->worker);
    self->user_data.init = NULL;
    self->user_data.name = NULL;
    self->user_data.self = NULL;
    self->worker.cb = mtdp_sink_routine;
    self->worker.args = self;
    self->input_pipe = input_pipe;
}

bool mtdp_sink_stop_requested(mtdp_sink_context* ctx)
{
    mtdp_sink_impl* self = (mtdp_sink_impl*) ((char*)(ctx) + offsetof(mtdp_sink_impl, context));
    return self->worker.destroyed || !self->worker.enabled;
}
