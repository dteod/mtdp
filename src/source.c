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

// clang-format off
#include "mtdp.h"
#include "impl/source.h"
#include "impl/pipe.h"
// clang-format on

#include "api.h"
#include "bell.h"
#include "thread.h"

#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__)
#  define likely(expr)   (__builtin_expect(!!(expr), 1))
#  define unlikely(expr) (__builtin_expect(!!(expr), 0))
#else
#  define likely(expr)   (expr)
#  define unlikely(expr) (expr)
#endif

static int
mtdp_source_routine(void* data)
{
    mtdp_source_impl* self = (mtdp_source_impl*)data;

    if(self->context.ready_to_push) {
        if(likely(mtdp_pipe_push_buffer(self->output_pipe, self->context.output))) {
            mtdp_semaphore_release(&self->output_pipe->semaphore, 1);
            self->context.output        = NULL;
            self->context.ready_to_push = false;
        }
        else {
            mtdp_set_done(&self->done);
            thrd_yield();
            return 0;
        }
    }
    if(!self->context.output) {
        self->context.output = mtdp_pipe_get_empty_buffer(self->output_pipe);
    }

    if(likely(self->context.output)) {
        if(unlikely(!self->initialized)) {
            if(self->user_data.init) {
                self->user_data.init(&self->context);
            }
            self->initialized = true;
        }
        self->user_data.process(&self->context);
    }
    else {
        thrd_yield();
    }
    return 0;
}

void
mtdp_source_create_thread(mtdp_source_impl* self)
{
    self->worker.name           = self->user_data.name;
    self->context.self          = self->user_data.self;
    self->context.ready_to_push = false;
    self->context.output        = NULL;
    self->done                  = 0;
    mtdp_worker_create_thread(&self->worker);
}

void
mtdp_source_destroy(mtdp_source_impl* self)
{
    mtdp_set_done(&self->done);
    mtdp_worker_destroy(&self->worker);
}

void
mtdp_source_configure(mtdp_source_impl* self, mtdp_pipe* output_pipe)
{
    mtdp_worker_init(&self->worker);
    self->initialized    = false;
    self->user_data.init = NULL;
    self->user_data.name = NULL;
    self->user_data.self = NULL;
    self->worker.cb      = mtdp_source_routine;
    self->worker.args    = self;
    self->output_pipe    = output_pipe;
}

MTDP_API_INTERNAL void
mtdp_source_finished(mtdp_source_context* ctx)
{
    mtdp_source_impl* self = (mtdp_source_impl*)((char*)(ctx) + offsetof(mtdp_source_impl, context));
    mtdp_set_done(&self->done);
    mtdp_worker_destroy(&self->worker);
}

MTDP_API_INTERNAL bool
mtdp_source_stop_requested(mtdp_source_context* ctx)
{
    mtdp_source_impl* self = (mtdp_source_impl*)((char*)(ctx) + offsetof(mtdp_source_impl, context));
    return self->worker.destroyed || !self->worker.enabled;
}
