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

#ifndef MTDP_IMPL_BUFFER_H
#define MTDP_IMPL_BUFFER_H

#include "memory.h"
#include "mtdp/buffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if MTDP_BUFFER_POOL_STATIC_SIZE
typedef struct {
    mtdp_buffer buffers[MTDP_BUFFER_POOL_STATIC_SIZE];
    size_t      size;
} mtdp_buffer_pool;
#else
typedef struct {
    mtdp_buffer* buffers;
    size_t       size, capacity;
} mtdp_buffer_pool;
#endif

void        mtdp_buffer_pool_init(mtdp_buffer_pool*);
void        mtdp_buffer_pool_destroy(mtdp_buffer_pool*);
bool        mtdp_buffer_pool_push_back(mtdp_buffer_pool* self, const mtdp_buffer e);
mtdp_buffer mtdp_buffer_pool_pop_back(mtdp_buffer_pool* self);
size_t      mtdp_buffer_pool_size(const mtdp_buffer_pool*);
bool        mtdp_buffer_pool_resize(mtdp_buffer_pool* self, size_t size);

typedef mtdp_buffer mtdp_buffer_fifo_block[MTDP_BUFFER_FIFO_BLOCK_SIZE];

#if MTDP_BUFFER_FIFO_BLOCKS
typedef mtdp_buffer_fifo_block mtdp_buffer_fifo_blocks[MTDP_BUFFER_FIFO_BLOCKS];
#else
/**
 * @brief To disambiguate between the block type and the pointer to the first element of a buffer block. 
 */
typedef mtdp_buffer* mtdp_buffer_aggregate;
#  if MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE
typedef struct {
    mtdp_buffer_aggregate blocks[MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE];
    size_t                size;
} mtdp_buffer_fifo_blocks;
#  else
typedef struct {
    mtdp_buffer_aggregate* blocks;
    size_t                 size, capacity;
} mtdp_buffer_fifo_blocks;
#  endif
#endif

/**
 * @brief Deque as implemented in C++ containing `mtdp_buffer`s.
 * 
 * @details The memory layout is as exposed in the following graph:
 *              ┌────────────┐
 *        deque │            │ blocks array
 *          ┌──────┐      ┌──▼──┐
 *          │blocks│  ┌───┤     │
 *          ├──────┤  │   ├─────┤
 *          │arr_sz│  │   │     │─────┐
 *          ├──────┤  │   ├─────┤     │
 *          │size  │  │   │     ├─────│────────────┐
 *          ├──────┤  │   └─────┘     │            │
 *          │offset│  │               │            │
 *          └──────┘  │               │            │
 *                    │               │            │
 *     block pool     │       ┌───────┘            │
 *   ┌────────────────┼───────┼──────────────────┐ │
 *   │    ┌───────────┘       │                  │ │
 *   │   ┌▼┬─┬─┬─┬─┬─┬─┬─┐   ┌▼┬─┬─┬─┬─┬─┬─┬─┐   │ │
 *   │   └─┴─┴◄┴─┴─┴─┴─┴─┘   └─┴─┴─┴─┴─┴►┴─┴─┘   │ │
 *   │        back                     front     │ │
 *   │                        ┌──────────────────┼─┘
 *   │   ┌---------------┐   ┌▼──────────────┐   │
 *   │   └---------------┘   └───────────────┘   │
 *   │    unclaimed block          free block    │
 *   └───────────────────────────────────────────┘
 * Depending on the settings, the following behaviors are customizable:
 * - the blocks array may or not be embedded in the buffer deque struct
 * - memory for the blocks may be stored in a static global pool,
 *   instead than requesting it from the heap.
 * 
 * The deque is used as a FIFO when embedded in a mtdp pipe. Data is
 * pushed on the back and pulled from the front, and the handling is performed
 * internally. The strategy aims at reducing the amount of allocations required
 * to operate the FIFO, shifting the elements when the fill ratio is more than
 * a predefined constant.
 */
typedef struct {
    mtdp_buffer_fifo_blocks blocks;
    size_t                  size;
    size_t                  first_element_offset;
} mtdp_buffer_fifo;

bool   mtdp_buffer_fifo_init(mtdp_buffer_fifo* self);
void   mtdp_buffer_fifo_destroy(mtdp_buffer_fifo* self);
bool   mtdp_buffer_fifo_push_back(mtdp_buffer_fifo* self, const mtdp_buffer);
bool   mtdp_buffer_fifo_pop_front(mtdp_buffer_fifo* self, mtdp_buffer*);
size_t mtdp_buffer_fifo_size(const mtdp_buffer_fifo* self);

#endif
