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

#ifndef MTDP_PIPELINE_H
#define MTDP_PIPELINE_H

#ifdef __cplusplus
#   include <cstdint>
#   include <cstddef>
    extern "C" {
#else
#   include <stdint.h>
#   include <stddef.h>
#endif

#include "mtdp/buffer.h"
#include "mtdp/pipe.h"
#include "mtdp/source.h"
#include "mtdp/stage.h"
#include "mtdp/sink.h"

/**
 * @brief Opaque struct used to manage a multi-threaded DSP pipeline.
 * 
 * @details A pipeline is a struct containing multiple threads operating
 * concurrently and synchronously on a data stream.
 * 
 * Three kinds of threads compose a pipeline, namely
 * - a source stage (always present)
 * - a user-configurable number of internal stages (0 is a valid number)
 * - a sink stage (always present)
 * 
 * Each stage is to be configured by the user to provide a callback
 * that will operate on the data stream in isolation and in a single-threaded
 * manner, together with some additional data the stage may use internally.
 * 
 * Pipes are used to handle access to the data in a synchronous manner
 * using user-configurable buffers that will be passed from a previous
 * stage to the next without contention.
 * 
 * Operation on the pipeline may be stopped and disabled both from the outside
 * - using the API provided in this header - or from the inside by the
 * source stage - using the `mtdp_source_finished` API (useful when operating
 * on files or other streams with a fixed length). In the former case
 * data already present in the pipeline and not already flushed by the sink
 * will be discarded, while in the latter case the pipeline will finish
 * processing data that was filled in the buffers until some processing may
 * be performed. `mtdp_pipeline_wait` may be used to wait on the pipeline
 * to finish execution before disabling it. Also, the pipeline may be
 * paused and resumed without data losses (the threads will be put in sleep),
 * but not from within the stage.
 * 
 * The pipeline can be seen as having three states:
 * - disabled: threads are not created, only data is allocated
 * - enabled: threads are created, but not operating
 * - active: threads are created and running
 * State transitions are handled by the API, a false
 * value is returned when performing an illegal operation and the 
 * library-owned errno is set accordingly to the actual fault.
 * 
 * @note All the functions used on this struct that somehow change
 * the pipeline internal state are not thread-safe.
 * Pipelines are designed to be configured and handled from a single
 * thread; if access from multiple threads is necessary, the user
 * shall provide its own synchronization mechanism.
 * The only allowed thread-safe mechanism is to wait on a pipeline
 * to finish execution without active state changes.
 */
typedef struct s_mtdp_pipeline mtdp_pipeline;

/**
 * @brief Struct used as pipeline creation parameters,
 * to be filled with configuration data.
 * 
 * @details This struct contains the actual data to be filled
 * by the user to instruct the creation of the pipeline.
 * 
 * @note This struct may be subject to changes in future
 * releases adding fields that may cover up to 1024 bytes.
 * Use the pipeline_parameters convenience union to update the
 * library (if shared) with the same ABI.
 */
typedef struct {
    /**
     * @brief Number of stages that the pipeline will contain.
     * 
     * @details This parameter sets the number of internal stages and pipes
     * the pipeline will allocate upon creation. The pipes will be 
     * 1 + this number, while the stages will equal this number. 0 is a
     * valid number.
     * 
     * @note Source and sink stages are not counted in this number, it
     * only describes the internal stages which are not a source nor a sink.
     */
    size_t internal_stages;
} mtdp_pipeline_params;

/**
 * @brief Convenience union used to pad the parameters struct,
 * for future implementations.
 * 
 * @details The library reserves itself 1024 bytes of data to be used for
 * input data configuration. This is to avoid breaking the library ABI
 * in future releases.
 */
typedef union {
    mtdp_pipeline_params params;
    uint8_t padding[1024];
} mtdp_pipeline_parameters;

/**
 * @brief Creates a multi-threaded DSP pipeline and returns a pointer to it.
 * 
 * @details This function will search for memory to allocate both the pipeline
 * and its internal structures, and it will return it after a preconfiguration
 * so that the next configuration steps will be to retrieve and fill
 * the source/stage/sink structures, to resize the (already allocated) pipes,
 * and to allocate the buffers.
 * 
 * Depending on how the library is configured, memory for the pipeline
 * and/or its internal will be retrieved from either 
 * a static global storage pool, or from the heap.
 * 
 * Future implementations reserve the possibility to customize the pipeline
 * behavior using runtime parameters provided with @p params .
 * 
 * @param params pointer to a pipeline_parameters creation structure
 * @return mtdp_pipeline* a pipeline to be configured, NULL on error
 * @retval MTDP_OK
 * @retval MTDP_NO_MEM
 */
mtdp_pipeline* mtdp_pipeline_create(const mtdp_pipeline_parameters* params);

/**
 * @brief Destroys a pipeline.
 * 
 * @details Effectively deallocates all memory allocated for the
 * pipeline. If the pipeline is active is will be disabled first.
 * 
 * @param pipeline the pipeline to destroy
 * @retval MTDP_OK
 * @retval MTDP_BAD_PTR
 */
void mtdp_pipeline_destroy(mtdp_pipeline *pipeline);

/**
 * @brief Returns the source stage of a pipeline.
 * 
 * @details The source stage shall be filled with user
 * data in order for the pipeline to operate it. Data can be
 * modified at runtime as long as the pipeline is not operating
 * (i.e. enabled and started).
 * 
 * @warning The user may retrieve the data, then start the pipeline, then
 * modify data via the returned pointer: this means requesting a data race.
 * 
 * @param pipeline the pipeline to retrieve the source stage from
 * @return mtdp_source* the source stage of the pipeline, NULL on error
 * @retval MTDP_OK
 * @retval MTDP_BAD_PTR
 * @retval MTDP_ACTIVE
 */
mtdp_source* mtdp_pipeline_get_source(mtdp_pipeline *pipeline);

/**
 * @brief Returns the array of internal stages of a pipeline.
 * 
 * @details The internal stages shall be filled with user
 * data in order for the pipeline to operate it. Data can be
 * modified at runtime as long as the pipeline is not operating
 * (i.e. enabled and started).
 * 
 * @note The stages array size equals the `internal_stages`
 * parameter used when creating the pipeline.
 * 
 * @warning The user may retrieve the data, then start the pipeline, then
 * modify data via the returned pointer: this means requesting a data race.
 * 
 * @param pipeline the pipeline to retrieve the stages from
 * @return mtdp_stage* the internal stages of the pipeline, NULL on error
 * @retval MTDP_OK
 * @retval MTDP_BAD_PTR
 * @retval MTDP_ACTIVE
 */
mtdp_stage* mtdp_pipeline_get_stages(mtdp_pipeline *pipeline);

/**
 * @brief Returns the sink stage of a pipeline.
 * 
 * @details The sink stage shall be filled with user
 * data in order for the pipeline to operate it. Data can be
 * modified at runtime as long as the pipeline is not operating
 * (i.e. enabled and started).
 * 
 * @warning The user may retrieve the data, then start the pipeline, then
 * modify data via the returned pointer: this means requesting a data race.
 * 
 * @param pipeline the pipeline to retrieve the sink stage from
 * @return mtdp_sink* the sink stage of the pipeline, NULL on error
 * @retval MTDP_OK
 * @retval MTDP_BAD_PTR
 * @retval MTDP_ACTIVE
 */
mtdp_sink* mtdp_pipeline_get_sink(mtdp_pipeline *pipeline);

/**
 * @brief Returns the array of pipes of a pipeline.
 * 
 * @details The pipes shall be resized by the user in order for
 * the pipeline to be fully operational. A pipe containing more buffers
 * may be used to account for stages with a higher latency.
 * After a resize, buffers shall also be allocated from the user.
 * Pipes may be resized at runtime as long as the pipeline is not operating
 * (i.e. enabled and started).
 * 
 * @note The pipes array size equals 1 + `internal_stages`, where
 * `internal_stages` is the parameter used when creating the pipeline.
 * 
 * @warning Resizing a pipe to a smaller value may lead to data loss.
 *  
 * @warning The user may retrieve the data, then start the pipeline, then
 * modify data via the returned pointer: this means requesting a data race.
 * 
 * @param pipeline the pipeline to retrieve the pipes from
 * @return mtdp_stage* the internal stages of the pipeline, NULL on error
 * @retval MTDP_OK
 * @retval MTDP_BAD_PTR
 * @retval MTDP_ACTIVE
 */
mtdp_pipe* mtdp_pipeline_get_pipes(mtdp_pipeline *pipeline);

/**
 * @brief Enables a pipeline.
 * 
 * @details Enabling a pipeline means storing data the user inserted in the
 * internal structures and creating the threads that will be used to process
 * data. Threads will be idle after a successful enable, and the pipeline
 * shall be started to become active.
 * 
 * @param pipeline the pipeline to enable
 * @return true on success, false on error
 * @retval MTDP_OK
 * @retval MTDP_BAD_PTR
 * @retval MTDP_ENABLED
 */
bool mtdp_pipeline_enable(mtdp_pipeline *pipeline);

/**
 * @brief Disables a pipeline.
 * 
 * @details Disabling a pipeline means destroying the threads on the stages
 * and clearing the pipes (i.e. putting back all the buffers in the pool).
 * This function works on both enabled pipelines and active pipelines.
 * 
 * @note If another thread is waiting on a pipeline to finish and it
 * is disabled, the thread waiting will resume execution.
 * 
 * @warning The order in which the buffers are put back in the pool
 * is not deterministic and it varies with the pipeline's internal
 * status: the cleared pipe will have an unsorted pool.
 * 
 * @param pipeline the pipeline to disable
 * @return true on success, false on error
 * @retval MTDP_OK
 * @retval MTDP_BAD_PTR
 * @retval MTDP_NOT_ENABLED
 */
bool mtdp_pipeline_disable(mtdp_pipeline *pipeline);

/**
 * @brief Starts a pipeline.
 * 
 * @details Starting a pipeline means waking up the threads previously put to
 * sleep to make them start working on the data stream.
 * 
 * @param pipeline the pipeline to start
 * @return true on success, false on error
 * @retval MTDP_OK
 * @retval MTDP_BAD_PTR
 * @retval MTDP_ACTIVE
 * @retval MTDP_NOT_ENABLED
 */
bool mtdp_pipeline_start(mtdp_pipeline *pipeline);

/**
 * @brief Stops a pipeline.
 * 
 * @details Stopping a pipeline means putting the threads to sleep
 * and returning to the `enabled' state.
 * 
 * @note Another thread waiting on the pipeline to finish will not
 * be resumed if this function is called.
 * 
 * @param pipeline the pipeline to stop
 * @return true on success, false on error
 * @retval MTDP_OK
 * @retval MTDP_BAD_PTR
 * @retval MTDP_ENABLED
 * @retval MTDP_NOT_ENABLED
 */
bool mtdp_pipeline_stop(mtdp_pipeline *pipeline);

/**
 * @brief Waits for a pipeline to finish execution.
 * 
 * @details This function will return when it will dynamically detect
 * any of these conditions:
 * - the pipeline is not enabled
 * - all the stages are not processing any data from at least 100 ms
 * The function does not return if the pipeline is in the enabled state
 * (i.e. the threads are sleeping).
 * 
 * @note This function is thread-safe, i.e. it may be called concurrently
 * on the same pipeline object from multiple threads, even while another
 * thread is actively changing the pipeline state.
 * 
 * @param pipeline the pipeline to wait
 * @retval MTDP_OK
 * @retval MTDP_BAD_PTR
 * @retval MTDP_NOT_ENABLED
 */
void mtdp_pipeline_wait(mtdp_pipeline *pipeline);

#ifdef __cplusplus
}
#endif

#endif
