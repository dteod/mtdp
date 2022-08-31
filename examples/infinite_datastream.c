/* Copyright (C) 2021-2022 Domenico Teodonio

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <sys/time.h>

#include <mtdp.h>

size_t timestamp_from(void* old)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec%86400)*1e6 + tv.tv_usec%1000000 - (old ? *(size_t*) old : 0UL);
}

typedef struct {
    size_t timestamp;
    size_t iteration_index;
} payload_data;

void long_task()
{
    unsigned long clock_cycles = 1e9;
    do {
        asm volatile ( "nop" );
    } while(clock_cycles--);
}

void source_payload(mtdp_source_context* ctx)
{
    payload_data* data = (payload_data*) ctx->self;
    printf("%10lu - [%10.6f] source(%lu)\n", data->iteration_index++,
        timestamp_from(&data->timestamp)/1e6, *(size_t*) ctx->output);
    long_task();
    ctx->ready_to_push = true;
}

void stage_payload(mtdp_stage_context* ctx)
{
    payload_data* data = (payload_data*) ctx->self;
    printf("%10lu - [%10.6f] stage(%lu, %lu)\n", data->iteration_index++, 
        timestamp_from(&data->timestamp)/1e6, *(size_t*) ctx->input, *(size_t*) ctx->output);
    long_task();
    ctx->ready_to_pull = ctx->ready_to_push = true;
}

void sink_payload(mtdp_sink_context* ctx)
{
    payload_data* data = (payload_data*) ctx->self;
    printf("%10lu - [%10.6f] sink(%lu)\n", data->iteration_index++, 
        timestamp_from(&data->timestamp)/1e6, *(size_t*) ctx->input);
    long_task();
    ctx->ready_to_pull = true;
}

int main()
{
    const size_t N_STAGES = 1;
    const size_t N_BUFFERS = 8;
    char c;
    register mtdp_pipeline *pipeline;
             mtdp_source   *source;
             mtdp_stage    *stages;
             mtdp_sink     *sink;
             mtdp_pipe     *pipe;
             mtdp_buffer   *buffers;

    /* 0. Retrieve memory for a pipeline and preconfigure it */
    mtdp_pipeline_parameters parameters;
    parameters.params.internal_stages = N_STAGES;
    pipeline = mtdp_pipeline_create(&parameters);
    if(!pipeline) {
        fprintf(stderr, "failed to create pipeline\n");
        exit(-1);
    }

    /* 1. Configure source, stage and sink with callback and user data.
          We also provide a name to the thread. */
    source = mtdp_pipeline_get_source(pipeline);
    source->name = "mtdp_source";
    source->init = NULL;
    source->process = source_payload;
    stages = mtdp_pipeline_get_stages(pipeline);
    stages[0].name = "mtdp_stage";
    stages[0].init = NULL;
    stages[0].process = stage_payload;
    sink = mtdp_pipeline_get_sink(pipeline);
    sink->name = "mtdp_sink";
    sink->init = NULL;
    sink->process = sink_payload;

    /* 2. Configure the pipes telling them how many buffers to handle, for each pipe.
          Also configure the buffers in the same cycle.
          These can be simple byte-level `malloc`s
          up to complex C++ classes created with `new`. */
    pipe = mtdp_pipeline_get_pipes(pipeline);
    for(size_t p = 0; p != 1 + N_STAGES; p++, pipe = mtdp_pipe_next(pipe)) {
        if(!mtdp_pipe_resize(pipe, N_BUFFERS)) {
            fprintf(stderr, "failed to allocate buffers for pipe %lu\n", p);
            exit(-1);
        }
        buffers = mtdp_pipe_buffers(pipe);
        for(size_t i = 0; i != N_BUFFERS; ++i) {
            buffers[i] = malloc(1024); /* Buffer creation */
            if(!buffers[i]) {
                fprintf(stderr, "failed to allocate buffer %lu on pipe %lu\n", i, p);
                exit(-1);
            }
            printf("allocated buffer[%lu] at %p\n", i, buffers[i]);
            ((size_t*) buffers[i])[0] = N_BUFFERS - i;
        }
    }

    /* 3. Fill the stages' data. Even here, data can be anything, 
          from a stack allocated plain struct to
          a huge C++ god class allocated on the heap. */
    payload_data source_data, stage_data, sink_data;
    source_data.timestamp =
    stage_data.timestamp  =
    sink_data.timestamp   =
        timestamp_from(NULL);
    source_data.iteration_index = 
    stage_data.iteration_index  = 
    sink_data.iteration_index   = 
        0;
    source->self    = &source_data;
    stages[0].self  = &stage_data;
    sink->self      = &sink_data;

    /* 4. Enable the pipeline to create the threads (but do not start them yet). */
    mtdp_pipeline_enable(pipeline);

    do {
        const size_t NSECONDS = 10;
        /* 5. Start the threads. Asynchronously wait while they operate. */
        mtdp_pipeline_start(pipeline);

        struct timespec ts;
        ts.tv_sec = NSECONDS;
        ts.tv_nsec = 0;
        nanosleep(&ts, NULL);

        /* 6. Stop the threads. */
        mtdp_pipeline_stop(pipeline);
        printf("pipeline stopped, you can check that the threads still exists and "
               "are now idle in htop or in the TaskManager\n");
        c = 0;
        while(!(c == 'r' || c == 'x' || c == 'q' || c == 'd')) {
            printf("insert a key:\n"
                "\tr to resume execution for another %lu seconds\n"
                "\tx to resume execution, wait 1 second, then destroy the pipeline\n"
                "\td to destroy the pipeline\n"
                "\tq to destroy the pipeline and quit\n"
            , NSECONDS);
            while((c = getchar()) == '\n') {}
        }
    } while(!(c == 'x' || c == 'q' || c == 'd'));

    if(c == 'x') {
        mtdp_pipeline_start(pipeline);

        struct timespec ts;
        ts.tv_sec = 1;
        ts.tv_nsec = 0;
        nanosleep(&ts, NULL);
    }
    
    /* 7. Destroy the threads and clear the pipes.
          We call this because the pipeline shall not be operating while
          we are deallocating buffers, or it may erroneously access them
          in the stages and segfault. */
    mtdp_pipeline_disable(pipeline);

    /* 8. Deallocate user-allocated memory. */
    pipe = mtdp_pipeline_get_pipes(pipeline);
    for(size_t p = 0; p != 1 + N_STAGES; ++p, pipe = mtdp_pipe_next(pipe)) {
        buffers = mtdp_pipe_buffers(pipe);
        for(size_t i = 0; i != N_BUFFERS; ++i) {
            printf("deleting buffer[%lu] at %p\n", i, buffers[i]);
            free(buffers[i]); /* Buffer destruction */
        }
    }

    /* 9. Destroy the pipeline and release all the memory it was carrying. */
    mtdp_pipeline_destroy(pipeline);

    if(c != 'q') {
        printf("pipeline destroyed, you can check that the threads no longer exist "
                "in htop or in the TaskManager\n");
        printf("press enter to quit\n");
        fflush(stdin);
        if(getchar() == '\n') {
            getchar();
        }
    }    

    /* 10. Think about what you can do with this library and enjoy! */
}
