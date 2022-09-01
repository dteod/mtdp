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

#if defined(_WIN32)
#   include <windows.h>
#   include <processthreadsapi.h>
enum {
    thrd_success,
    thrd_busy,
    thrd_error,
    thrd_nomem,
    thrd_timedout
};
#   define thrd_t HANDLE
#   define thrd_current() GetCurrentThread()
#   define thrd_create(t, f, data) (*(t) = CreateThread(NULL, 0, f, data, 0, NULL), ((t) ? thrd_success : ((GetLastError() == ERROR_NOT_ENOUGH_MEMORY) ? thrd_nomem : thrd_error)))
inline int thrd_join(thrd_t t, int* ret)
{
    int out = thrd_success;
    WaitForSingleObject(t, INFINITE);
    if (GetLastError() != ERROR_SUCCESS) {
        out = thrd_error;
    }
    if (ret) {
        DWORD uret;
        GetExitCodeThread(t, &uret);
        *ret = (int)uret;
        if (GetLastError() != ERROR_SUCCESS) {
            out = thrd_error;
        }
    }
    return out;
}
#   define nop() __nop()
#else
#   include <time.h>
#   include <sys/time.h>
#   include <threads.h>
#   define nop() do {asm volatile("nop");} while(0)
#endif

#include <mtdp.h>

size_t timestamp_from(void* old)
{
#if defined(_WIN32)
    static bool init = false;
    static ULONGLONG programStart = 0;
    FILETIME filetime;
    ULARGE_INTEGER largeInteger;

    GetSystemTimePreciseAsFileTime(&filetime);
    largeInteger.LowPart = filetime.dwLowDateTime;
    largeInteger.HighPart = filetime.dwHighDateTime;

    if (!init) {
        programStart = largeInteger.QuadPart;
        largeInteger.QuadPart = 0;
        init = true;
    }
    else {
        largeInteger.QuadPart -= programStart;
    }
    return (unsigned long)largeInteger.QuadPart / 10 + (largeInteger.QuadPart % 10 > 5) - (old ? *(size_t*)old : 0UL);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec % 86400) * 1e6 + tv.tv_usec % 1000000 - (old ? *(size_t*)old : 0UL);
#endif
}

typedef struct {
    size_t timestamp;
    size_t iteration_index;
} payload_data;

void long_task()
{
    unsigned long clock_cycles = (unsigned long) 1e9;
    do {
        nop();
    } while (clock_cycles--);
}

void source_payload(mtdp_source_context* ctx)
{
    payload_data* data = (payload_data*)ctx->self;
    printf("%10zu - [%10.6f] source(%zu)\n", data->iteration_index++,
        timestamp_from(&data->timestamp) / 1e6, *(size_t*)ctx->output);
    long_task();

    if (timestamp_from(&data->timestamp) / 1e6 > 5) {
        /*
            Here the source stage will notify that it finished.
            It will be destroyed at the very next iteration,
            while the next stages will notify their inactivity
            when they will finish process the incoming buffers.

            In this way data streams with a fixed size can be
            analyzed without hacking solutions like "wait this
            amount of time depending on the file size and the
            throughput of my processing". Come on.
        */
        printf("source stage finished\n");
        mtdp_source_finished(ctx);
    }
    ctx->ready_to_push = true;
}

void stage_payload(mtdp_stage_context* ctx)
{
    payload_data* data = (payload_data*)ctx->self;
    printf("%10zu - [%10.6f] stage(%zu, %zu)\n", data->iteration_index++,
        timestamp_from(&data->timestamp) / 1e6, *(size_t*)ctx->input, *(size_t*)ctx->output);
    long_task();
    ctx->ready_to_pull = ctx->ready_to_push = true;
}

void sink_payload(mtdp_sink_context* ctx)
{
    payload_data* data = (payload_data*)ctx->self;
    printf("%10zu - [%10.6f] sink(%zu)\n", data->iteration_index++,
        timestamp_from(&data->timestamp) / 1e6, *(size_t*)ctx->input);
    long_task();
    ctx->ready_to_pull = true;
}

#if defined(_WIN32)
DWORD wait_on(void* data)
#elif defined(__unix__)
int wait_on(void* data)
#endif
{
    printf("%zu: started\n", (size_t) thrd_current());
    mtdp_pipeline* pipeline = (mtdp_pipeline*)data;
    mtdp_pipeline_wait(pipeline);
    printf("%zu: exiting\n", (size_t) thrd_current());
    return 0;
}

int main()
{
    const size_t N_STAGES = 1;
    const size_t N_BUFFERS = 32;
    register mtdp_pipeline* pipeline;
    mtdp_source* source;
    mtdp_stage* stages;
    mtdp_sink* sink;
    mtdp_pipe* pipe;
    mtdp_buffer* buffers;

    /* 0. Retrieve memory for a pipeline and preconfigure it */
    mtdp_pipeline_parameters parameters;
    parameters.params.internal_stages = N_STAGES;
    pipeline = mtdp_pipeline_create(&parameters);
    if (!pipeline) {
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
    for (size_t p = 0; p != 1 + N_STAGES; p++, pipe = mtdp_pipe_next(pipe)) {
        if (!mtdp_pipe_resize(pipe, N_BUFFERS)) {
            fprintf(stderr, "failed to allocate buffers for pipe %zu\n", p);
            exit(-1);
        }
        buffers = mtdp_pipe_buffers(pipe);
        for (size_t i = 0; i != N_BUFFERS; ++i) {
            buffers[i] = malloc(1024); /* Buffer creation */
            if (!buffers[i]) {
                fprintf(stderr, "failed to allocate buffer %zu on pipe %zu\n", i, p);
                exit(-1);
            }
            printf("allocated buffer[%zu] at %p\n", i, buffers[i]);
            ((size_t*)buffers[i])[0] = N_BUFFERS - i;
        }
    }

    /* 3. Fill the stages' data. Even here, data can be anything,
          from a stack allocated plain struct to
          a huge C++ god class allocated on the heap. */
    payload_data source_data, stage_data, sink_data;
    source_data.timestamp =
        stage_data.timestamp =
        sink_data.timestamp =
        timestamp_from(NULL);
    source_data.iteration_index =
        stage_data.iteration_index =
        sink_data.iteration_index =
        0;
    source->self = &source_data;
    stages[0].self = &stage_data;
    sink->self = &sink_data;

    /* 4. Enable the pipeline to create the threads (but do not start them yet). */
    mtdp_pipeline_enable(pipeline);
    mtdp_pipeline_start(pipeline);

    /* 5. Create three new threads and make them wait the pipeline to finish. */
    thrd_t t1;
    thrd_t t2;
    thrd_t t3;

    thrd_create(&t1, wait_on, pipeline);
    thrd_create(&t2, wait_on, pipeline);
    thrd_create(&t3, wait_on, pipeline);

    printf("threads waiting for pipeline to finish, stopping for 5 seconds after 2 seconds\n");

    /* 6. Playing a bit with timings. You can tweak with this example to see how it behaves. */
    unsigned int seconds_to_sleep = 2;
#if defined(_WIN32)
    Sleep(seconds_to_sleep * 1000);
#elif defined(__unix__)
    struct timespec ts;
    ts.tv_sec = seconds_to_sleep;
    ts.tv_nsec = 0;
    nanosleep(&ts, NULL);
#endif
    mtdp_pipeline_stop(pipeline);
    printf("pipeline stopped\n");

    seconds_to_sleep = 5;
#if defined(_WIN32)
    Sleep(seconds_to_sleep * 1000);
#elif defined(__unix__)
    ts.tv_sec = seconds_to_sleep;
    ts.tv_nsec = 0;
    nanosleep(&ts, NULL);
#endif
    printf("%s",
        "activating the pipeline again -> the source stage will detect\n"
        "that 5 seconds are passed since the beginning and it will notify\n"
        "that it finished. It will be destroyed, as well.\n"
        "The rest of the stages will continue execution\n"
        "until data is available in the pipeline. As soon as data will be fully processed,\n"
        "threads will acknowledge the pipeline inactivity and exit almost immediately.\n"
        "The MTDP_PIPELINE_CONSUMER_TIMEOUT_US compile setting may be configured for the timeout\n"
        "after which the stages will signal that no input is being produced from the\n"
        "previous stage. The default is 100 ms.\n"
    );
    mtdp_pipeline_start(pipeline);

    thrd_join(t1, NULL);
    thrd_join(t2, NULL);
    thrd_join(t3, NULL);

    /* 7. A pipeline that finished execution autonomously still has to be disabled manually. */
    printf("disabling pipeline\n");
    mtdp_pipeline_disable(pipeline);

    /* 8. Remember to deallocate the user buffers. */
    pipe = mtdp_pipeline_get_pipes(pipeline);
    for (size_t p = 0; p != 1 + N_STAGES; ++p, pipe = mtdp_pipe_next(pipe)) {
        buffers = mtdp_pipe_buffers(pipe);
        for (size_t i = 0; i != N_BUFFERS; ++i) {
            printf("deleting buffer[%zu] at %p\n", i, buffers[i]);
            free(buffers[i]); /* Buffer destruction */
        }
    }

    /* 9. Destroy the pipeline and release all the memory it was carrying. */
    mtdp_pipeline_destroy(pipeline);

    /* 10. Think about what you can do with this library and enjoy! */
}
