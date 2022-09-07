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

#ifndef MTDP_PIPE_H
#define MTDP_PIPE_H

#ifndef MTDP_H
#   error do not #include <mtdp/buffer.h> directly, #include <mtdp.h> instead
#endif

/** 
 * @file 
 * 
 * @brief Header containing the pipe opaque struct and APIs to interact with it.
 * @note Do not import this file in user code, use the mtdp.h umbrella header instead.
 */

#include "mtdp/buffer.h"

/**
 * @brief Opaque struct used to configure a pipe.
 * 
 * @details A pipe is a channel used to connect two pipeline stages.
 * Pipe memory is contained in libmtdp, though the actual buffers
 * to be used from within the stages shall be configured externally.
 * This opaque struct together with its API can be used
 * to internally allocate and then retrieve the buffers
 * for initialization and cleanup.
 * 
 * The general use-case is as following
 * (dummy allocation/deallocation functions used for example):
 * @code {.c}
 * void my_buf_init(mtdp_buffer* buf);
 * void my_buf_destroy(mtdp_buffer* buf);
 * 
 * int main()
 * {
 *     register_signals();
 *     mtdp_pipeline* pipeline = mtdp_pipeline_create(STAGES);
 *     // ... 
 *     mtdp_buffer* buf;
 *     mtdp_pipe* pipes_head = mtdp_pipeline_get_pipes(pipeline);
 *     for(int i = STAGES; --i; pipes_head = mtdp_pipe_next(pipes_head)) {
 *         buf = mtdp_pipe_resize(pipes_head, nbufs = NBUFS);
 *         if(buf) {
 *             for(; --nbufs; buf++) {
 *                 my_buf_init(buf);
 *             }
 *         }
 *         else {
 *             fprintf("memory allocation error\n");
 *             exit(-1);
 *         }
 *     }
 *     // ...
 *     mtdp_pipeline_enable(pipeline);
 *     mtdp_pipeline_start(pipeline);
 *     // ...
 *     mtdp_pipeline_disable(pipeline);
 *     // ...
 *     pipes_head = mtdp_pipeline_pipes(pipeline);
 *     for(int i = STAGES + 1; i--; pipes_head = mtdp_pipe_next(pipes_head)) {
 *         for(buf = mtdp_pipe_buffers(pipes_head); --nbufs; buf++)) {
 *             my_buf_destroy(buf);
 *         }
 *     }
 *     mtdp_pipeline_destroy(pipeline);
 * }
 * @endcode
 */
typedef struct s_mtdp_pipe mtdp_pipe;

/**
 * @brief Returns the next pipe entry from a previous entry.
 * 
 * @details mtdp_pipe is an opaque type, so its size is not to be exposed
 * in client code. Use this function to retrieve the next entry.
 * 
 * @warning No bounds checking is performed. The number of times
 * this function may be called equals the `internal_stages` parameter
 * used to instantiate the pipeline.
 * 
 * @param pipe the pipe to retrieve the next entry from
 * @return mtdp_pipe* the pipe after the one given as a parameter
 */
mtdp_pipe* mtdp_pipe_next(mtdp_pipe* pipe);

/**
 * @brief Resizes the pipe's internal buffers.
 * 
 * @details Depending on the requested number of buffers, this function will:
 * - expand the pool of empty buffers, if more space is required;
 * - do nothing if the actual total number of buffers equals @p n_buffers;
 * - remove buffers from its internal structures with these priorities:
 *      1. shrink the pool of empty buffers;
 *      2. remove the fifo entries from the oldest one,
 *         if the buffer reduction exceeds the number of empty buffers;
 * 
 * @note This function is not thread-safe, and it should not be called
 * while the pipeline is active. It is only useful to preallocate memory
 * or to expand it at runtime (after pausing the pipeline).
 * 
 * @warning If a smaller number of buffers is requested, these buffers
 * owned memory, and they were not properly destructed, that memory will leak.
 * Always shrink after proper buffer destruction.
 * 
 * @warning A data reduction while the pipeline is operating
 * may lead to a data loss if not in a `malloc`d environment.
 * 
 * @param pipe the pipe to resize
 * @param n_buffers the number of buffers required for the resize
 * 
 * @return mtdp_buffer* a pointer to an array of mtdp_buffer, NULL on error
 * @retval MTDP_OK
 * @retval MTDP_BAD_PTR
 * @retval MTDP_NO_MEM
 */
mtdp_buffer* mtdp_pipe_resize(mtdp_pipe *pipe, size_t n_buffers);

/**
 * @brief Returns the currently empty buffers' pool of a pipe.
 * 
 * @details Returns a pointer to the first element of a random-access array.
 * Use this function to initialize the buffers after a resize,
 * but before enabling the pipeline.
 * 
 * @warning Even though this function is thread-safe, accessing the buffers
 * while the pipeline is operating will more likely than not corrupt the data,
 * or cause a segfault in a `malloc`d runtime. So avoid that.
 * 
 * @param pipe the pipe to retrieve the buffers from
 * @return mtdp_buffer* a linear array of buffers whose size
 * should have been previously set by the user, NULL on error or if size is 0
 * @retval MTDP_OK
 * @retval MTDP_BAD_PTR
 */
mtdp_buffer* mtdp_pipe_buffers(mtdp_pipe *pipe);

#endif
