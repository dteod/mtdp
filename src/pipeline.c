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

#include "mtdp/pipeline.h"
#include "impl/pipeline.h"
#include "impl/errno.h"

#include "futex.h"
#include "bell.h"
#include "memory.h"

#if MTDP_PIPELINE_STATIC_INSTANCES
#   if MTDP_PIPELINE_STATIC_INSTANCES > 0
        MTDP_DEFINE_STATIC_INSTANCES(mtdp_pipeline, mtdp_static_pipelines, MTDP_PIPELINE_STATIC_INSTANCES)
#   else
        MTDP_DEFINE_STATIC_INSTANCE(mtdp_pipeline, mtdp_static_pipeline)
#   endif
#else
    MTDP_DEFINE_DYNAMIC_INSTANCE(mtdp_pipeline)
#endif

static void mtdp_pipeline_configure(mtdp_pipeline* pipeline)
{
    mtdp_source_configure(&pipeline->source_impl, &pipeline->pipes[0]);
    for(size_t i = 0; i != pipeline->n_stages; ++i) {
        mtdp_stage_configure(&pipeline->stage_impls[i], &pipeline->pipes[i], &pipeline->pipes[i+1], &pipeline->stages[i]);
    }
    mtdp_sink_configure(&pipeline->sink_impl, &pipeline->pipes[pipeline->n_stages]);
    pipeline->enabled = false;
    pipeline->active = false;
    pipeline->destroying = 0;
}

static void mtdp_pipeline_join(mtdp_pipeline* pipeline)
{
    mtdp_worker_join(&pipeline->source_impl.worker);
    for(size_t n_stages = 0; n_stages != pipeline->n_stages; ++n_stages) {
        mtdp_worker_join(&pipeline->stage_impls[n_stages].worker);
    }
    mtdp_worker_join(&pipeline->sink_impl.worker);
}

static void mtdp_pipeline_clear(mtdp_pipeline *self)
{
    for(size_t i = self->n_stages + 1; i--; ) {
        mtdp_pipe_clear(&self->pipes[i]);
    }
    if(self->sink_impl.context.input) {
        mtdp_pipe_put_back(&self->pipes[self->n_stages], self->sink_impl.context.input);
        self->sink_impl.context.input = NULL;
    }
    for(size_t i = self->n_stages; i--; ) {
        if(self->stage_impls[i].context.input) {
            mtdp_pipe_put_back(&self->pipes[i], self->stage_impls[i].context.input);
            self->stage_impls[i].context.input = NULL;
        }
        if(self->stage_impls[i].context.output) {
            mtdp_pipe_put_back(&self->pipes[i+1], self->stage_impls[i].context.output);
            self->stage_impls[i].context.output = NULL;
        }
    }
    if(self->source_impl.context.output) {
        mtdp_pipe_put_back(&self->pipes[0], self->source_impl.context.output);
        self->source_impl.context.output = NULL;
    }
}

mtdp_pipeline* mtdp_pipeline_create(const mtdp_pipeline_parameters* parameters)
{
    mtdp_pipeline* out = mtdp_pipeline_alloc();
    if(out) {
        if(!mtdp_stage_vector_resize(&out->stages, parameters->params.internal_stages)) {
            mtdp_pipeline_dealloc(out);
            mtdp_errno_location = MTDP_NO_MEM;
            return NULL;
        }
        if(!mtdp_stage_impl_vector_resize(&out->stage_impls, parameters->params.internal_stages)) {
            mtdp_stage_vector_destroy(&out->stages);
            mtdp_pipeline_dealloc(out);
            mtdp_errno_location = MTDP_NO_MEM;
            return NULL;
        }
        if(!mtdp_pipe_vector_resize(&out->pipes, parameters->params.internal_stages + 1)) {
            mtdp_stage_impl_vector_destroy(&out->stage_impls);
            mtdp_stage_vector_destroy(&out->stages);
            mtdp_pipeline_dealloc(out);
            mtdp_errno_location = MTDP_NO_MEM;
            return NULL;
        }
        for(size_t i = 0; i < parameters->params.internal_stages + 1; ++i) {
            if(!mtdp_pipe_init(&out->pipes[i])) {
                for(size_t j = 0; j < i; ++j) {
                    mtdp_pipe_destroy(&out->pipes[j]);
                }
                mtdp_pipe_vector_destroy(&out->pipes);
                mtdp_stage_impl_vector_destroy(&out->stage_impls);
                mtdp_stage_vector_destroy(&out->stages);
                mtdp_pipeline_dealloc(out);
                mtdp_errno_location = MTDP_NO_MEM;
                return NULL;
            }
        }
        out->n_stages = parameters->params.internal_stages;
        mtdp_pipeline_configure(out);
        mtdp_errno_location = MTDP_OK;
    } else {
        mtdp_errno_location = MTDP_BAD_PTR;
    }
    return out;
}

void mtdp_pipeline_destroy(mtdp_pipeline *pipeline)
{
    if(pipeline) {
        mtdp_pipeline_disable(pipeline);
        for(size_t i = 0; i < 1 + pipeline->n_stages; ++i) {
            mtdp_pipe_destroy(&pipeline->pipes[i]);
        }
        mtdp_pipe_vector_destroy(&pipeline->pipes);
        mtdp_stage_impl_vector_destroy(&pipeline->stage_impls);
        mtdp_stage_vector_destroy(&pipeline->stages);
        mtdp_pipeline_dealloc(pipeline);
        mtdp_errno_location = MTDP_OK;
    } else {
        mtdp_errno_location = MTDP_BAD_PTR;
    }
}

mtdp_source* mtdp_pipeline_get_source(mtdp_pipeline* pipeline)
{
    return pipeline ? &pipeline->source_impl.user_data : NULL;
}

mtdp_stage* mtdp_pipeline_get_stages(mtdp_pipeline* pipeline)
{
    return pipeline ? pipeline->stages : NULL;
}

mtdp_sink* mtdp_pipeline_get_sink(mtdp_pipeline* pipeline)
{
    return pipeline ? &pipeline->sink_impl.user_data : NULL;
}

mtdp_pipe* mtdp_pipeline_get_pipes(mtdp_pipeline* pipeline)
{
    return pipeline ? pipeline->pipes : NULL;
}

bool mtdp_pipeline_enable(mtdp_pipeline* pipeline)
{
    if(pipeline) {
        if(!pipeline->enabled) {
            mtdp_sink_create_thread(&pipeline->sink_impl);
            for(size_t i = pipeline->n_stages; i--; ) {
                mtdp_stage_create_thread(&pipeline->stage_impls[i]);
            }
            mtdp_source_create_thread(&pipeline->source_impl);
            pipeline->enabled = true;
            pipeline->active = false;
            mtdp_errno_location = MTDP_OK;
            return true;
        } else {
            mtdp_errno_location = MTDP_ENABLED;
        }
    } else {
        mtdp_errno_location = MTDP_BAD_PTR;
    }
    return false;
}

bool mtdp_pipeline_disable(mtdp_pipeline* pipeline)
{
    if(pipeline) {
        if(pipeline->enabled) {
            mtdp_set_done(&pipeline->destroying);
            mtdp_source_destroy(&pipeline->source_impl);
            for(size_t i = 0; i != pipeline->n_stages; ++i) {
                mtdp_stage_destroy(&pipeline->stage_impls[i]);
            }
            mtdp_sink_destroy(&pipeline->sink_impl);
            mtdp_pipeline_join(pipeline);
            mtdp_pipeline_clear(pipeline);
            for(size_t i = 0; i < pipeline->n_stages; ++i) {
                mtdp_set_done(&pipeline->stage_impls[i].done);
            }
            pipeline->active = false;
            pipeline->enabled = false;
            mtdp_unset_done(&pipeline->destroying);
            mtdp_errno_location = MTDP_OK;
            return true;
        } else {
            mtdp_errno_location = MTDP_NOT_ENABLED;
        }
    } else {
        mtdp_errno_location = MTDP_BAD_PTR;
    }
    return false;
}

bool mtdp_pipeline_start(mtdp_pipeline* pipeline)
{
    if(pipeline) {
        if(pipeline->enabled) {
            if(pipeline->active) {
                mtdp_errno_location = MTDP_ACTIVE;
            } else {
                mtdp_worker_enable(&pipeline->sink_impl.worker);
                for(size_t i = pipeline->n_stages; i--; ) {
                    mtdp_worker_enable(&pipeline->stage_impls[i].worker);
                }
                mtdp_worker_enable(&pipeline->source_impl.worker);
                pipeline->active = true;
                mtdp_errno_location = MTDP_OK;
                return true;
            }
        } else {
            mtdp_errno_location = MTDP_NOT_ENABLED;
        }
    } else {
        mtdp_errno_location = MTDP_BAD_PTR;
    }
    return false;
}

bool mtdp_pipeline_stop(mtdp_pipeline* pipeline)
{
    if(pipeline) {
        if(pipeline->enabled) {
            if(!pipeline->active) {
                mtdp_errno_location = MTDP_ENABLED;
            } else {
                mtdp_worker_disable(&pipeline->source_impl.worker);
                for(size_t i = 0; i != pipeline->n_stages; ++i) {
                    mtdp_worker_disable(&pipeline->stage_impls[i].worker);
                }
                mtdp_worker_disable(&pipeline->sink_impl.worker);
                mtdp_errno_location = MTDP_OK;
                pipeline->active = false;
                return true;
            }
        } else {
            mtdp_errno_location = MTDP_NOT_ENABLED;
        }
    } else {
        mtdp_errno_location = MTDP_BAD_PTR;
    }
    return false;
}

void mtdp_pipeline_wait(mtdp_pipeline *pipeline)
{
    bool exit;

    if(pipeline) {
        mtdp_futex_wait(&pipeline->destroying, 1);
        if(pipeline->enabled) {
            do {
                mtdp_futex_wait(&pipeline->source_impl.done, 0);
                for(size_t i = 0; i != pipeline->n_stages; ++i) {
                    mtdp_futex_wait(&pipeline->stage_impls[i].done, 0);
                }
                mtdp_futex_wait(&pipeline->sink_impl.done, 0);
                exit = pipeline->source_impl.done != 0;
                for(size_t i = 0; exit && i != pipeline->n_stages; ++i) {
                    exit &= pipeline->stage_impls[i].done != 0;
                }
                exit &= pipeline->sink_impl.done != 0;
            } while(!exit);
            mtdp_errno_location = MTDP_OK;
        } else {
            mtdp_errno_location = MTDP_NOT_ENABLED;
        }
    } else {
        mtdp_errno_location = MTDP_BAD_PTR;
    }
}
