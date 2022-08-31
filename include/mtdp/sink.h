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

/** 
 * @file 
 * 
 * @brief Header containing the mtdp_sink struct, API and related utilities.
 * 
 * @details Avoid importing this file in user code, prefer the mtdp.h umbrella header.
*/

#ifndef MTDP_SINK_H
#define MTDP_SINK_H

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
 * @brief Convenience wrapper around data given to a sink.
 */
typedef void* mtdp_sink_data;

/**
 * @brief Data to be used by the sink callback at every iteration.
 */
typedef struct {
    /**
     * @brief User-provided data.
     * 
     * @details Data given here may be stack allocated or a huge
     * C++ class created with new. It is copied from the user-provided
     * data on the sink context every time the pipeline is enabled.
     */
    mtdp_sink_data       self;

    /**
     * @brief The input buffer coming from the pipe.
     * 
     * @details This buffer will be set by the library and it will
     * be synchronous wrt the previously created buffers. Set the
     * @p ready_to_pull parameter to instruct the pipeline to wait for a
     * new buffer coming from the previous stage.
     * 
     * @warning Do not deallocate it from within the callback, or you will
     * pollute the pipe with deallocated data that will segfault as
     * soon as they will be accessed from the previous stages.
     */
    mtdp_buffer          input;

    /**
     * @brief Set this to tell the library to give you a new input buffer.
     * 
     * @details The sink routine contained in the library will check
     * this value before every iteration and, if enabled, it will set
     * the @p input buffer with a new buffer coming from the input FIFO.
     * The value will be reset after pulling a buffer from the input FIFO.
     * 
     * @note If you do not set this flag in the sink callback, you are
     * effectively telling the library that you did not finish with
     * the previous buffer, that you do not need any more buffers yet,
     * thus reducing the sink throughput to zero.
     */
    bool                 ready_to_pull;
} mtdp_sink_context;

/**
 * @brief The sink callback accepts a single parameter, that is its context.
 */
typedef void(*mtdp_sink_callback)(mtdp_sink_context*);

/**
 * @brief Struct to be filled by the user with data describing the sink stage.
 * 
 * @details This struct is to be filled by the user with data describing
 * how the sink will process the buffers, together with some additional
 * data the sink may use. Not all of the fields are to be provided:
 * check the documentation of the fields for additional information.
 */
typedef struct {
    /**
     * @brief This field will name the thread running the sink callback.
     * 
     * @details Naming a thread is useful for both debugging and production
     * runtime monitoring. It is optional to set: by default, the thread name
     * will not be set, and the default behavior is platform dependent (often
     * it will inherit the name of the thread enabling the pipeline).
     */
    const char*          name;

    /**
     * @brief User data provided to the sink.
     * 
     * @details Even though for simple stages this may be an overkill,
     * bigger and more complex stages may incorporate mechanisms to
     * communicate partial results to external threads, or even just
     * to provide a configuration mechanism. It is optional to set.
     */
    mtdp_sink_data       self;

    /**
     * @brief Sink initialization function.
     * 
     * @details This function, if set, will be called only once after
     * the pipeline will be enabled right before the first sink iteration.
     * It may be used e.g. for initializing scratch data directly from within
     * the sink callback on/with the @p self parameter. It is optional to set.
     */
    mtdp_sink_callback   init;

    /**
     * @brief Callback to be called by the sink stage on every iteration.
     * 
     * @details This callback will be called on a context containing
     * freshly produced buffers - when the previous stages will provide any-,
     * together with some user data.
     * It is required to set, since it is the core of the whole processing
     * mechanism.
     * 
     * @note Remember to provide a function that repeatedly sets the 
     * @p ready_to_pull parameter in the sink context or the input buffers
     * will not be updated and the sink thoughput will be zeroed.
     */
    mtdp_sink_callback   process;
} mtdp_sink;

/**
 * @brief This function may be called from within the sink callback to
 * understand if a stop or a disable was requested from the pipeline,
 * in order to prehemptively stop any long-lasting processings.
 * 
 * @return bool wether a stop was requested (true) or not (false)
 */
bool mtdp_sink_stop_requested(mtdp_sink_context*);

#ifdef __cplusplus
}
#endif

#endif
