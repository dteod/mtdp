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

#ifndef MTDP_SOURCE_H
#define MTDP_SOURCE_H

#ifdef __cplusplus
#   include <cstdint>
#   include <cstddef>
    extern "C" {
#else
#   include <stdint.h>
#   include <stddef.h>
#   include <stdbool.h>
#endif

#include "mtdp/buffer.h"

/**
 * @brief Convenience wrapper around data given to a source.
 */
typedef void* mtdp_source_data;

/**
 * @brief Data to be used by the source callback at every iteration.
 */
typedef struct {
    /**
     * @brief User-provided data.
     * 
     * @details Data given here may be stack allocated or a huge
     * C++ class created with new. It is copied from the user-provided
     * data on the source context every time the pipeline is enabled.
     */
    mtdp_source_data     self;
    mtdp_buffer          output;

    /**
     * @brief Set this to tell the library to push a buffer on the output
     * pipe's FIFO before the next iteration.
     * 
     * @details The source routine contained in the library will check
     * this value before every iteration and, if enabled, it will push
     * the @p output buffer on the output FIFO and it will retrieve a
     * new empty buffer before the next iteration, and then reset it.
     * 
     * @note If you do not set this flag in the source callback, you are
     * effectively telling the library that you did not finish with
     * the buffer, that you do are not ready to push the buffer yet,
     * thus reducing the source throughput to zero.
     */
    bool                 ready_to_push;
} mtdp_source_context;

/**
 * @brief The source callback accepts a single parameter, that is its context.
 */
typedef void(*mtdp_source_callback)(mtdp_source_context*);

/**
 * @brief Struct to be filled with user data.
 * 
 * @details This struct is to be filled by the user with data describing
 * how the source will process the buffers, together with some additional
 * data the source may use. Not all of the fields are to be provided:
 * check the documentation of the fields for additional information.
 */
typedef struct {
    /**
     * @brief This field will name the thread running the source callback.
     * 
     * @details Naming a thread is useful for both debugging and production
     * runtime monitoring. It is optional to set: by default, the thread name
     * will not be set, and the default behavior is platform dependent (often
     * it will inherit the name of the thread enabling the pipeline).
     */
    const char*          name;

    /**
     * @brief User data provided to the source.
     * 
     * @details Even though for simple stages this may be an overkill,
     * bigger and more complex stages may incorporate mechanisms to
     * communicate partial results to external threads, or even just
     * to provide a configuration mechnism. It is optional to set.
     */
    mtdp_source_data      self;

    /**
     * @brief Source initialization function.
     * 
     * @details This function, if set, will be called only once after
     * the pipeline will be enabled right before the first source iteration.
     * It may be used e.g. for initializing scratch data directly from within
     * the source callback on/with the @p self parameter. It is optional to set.
     */
    mtdp_source_callback init;

    /**
     * @brief Callback to be called by the source stage on every iteration.
     * 
     * @details This callback will be called on a context containing 
     * an unused output buffer retrieved from the output pipe together with 
     * some user data. It is required to set, since it is the core
     * of the whole processing mechanism.
     * 
     * @note Remember to provide a function that repeatedly sets the 
     * @p ready_to_pull parameter in the source context or the input buffers
     * will not be updated and the source thoughput will be zeroed.
     */
    mtdp_source_callback process;
} mtdp_source;

void mtdp_source_finished(mtdp_source_context*);

/**
 * @brief This function may be called from within the source callback to
 * understand if a stop or a disable was requested from the pipeline,
 * in order to prehemptively stop any long-lasting processings.
 * 
 * @return bool wether a stop was requested (true) or not (false)
 */
bool mtdp_source_stop_requested(mtdp_source_context*);

#ifdef __cplusplus
}
#endif

#endif
