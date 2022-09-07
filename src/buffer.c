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

#include <math.h>

#include "mtdp.h"
#include "impl/buffer.h"

#if MTDP_BUFFER_FIFO_BLOCK_SIZE == 0
#   error The block size for the buffer deque shall be non-zero
#endif

#if !MTDP_BUFFER_POOL_STATIC_SIZE
#   include <stdlib.h>
#   include <string.h>
    static inline bool mtdp_buffer_pool_realloc(mtdp_buffer_pool* self, size_t elements) {
        mtdp_buffer* tmp;

        tmp = (mtdp_buffer*) (self->capacity ? 
            realloc(self->buffers, elements*sizeof(mtdp_buffer))
            : calloc(elements, sizeof(mtdp_buffer)));
        if(tmp) {
            self->capacity = elements;
            self->buffers = tmp;
        }
        return !!tmp;
    }

    static inline bool mtdp_buffer_pool_has_space_for(const mtdp_buffer_pool* self, size_t elements) {
        return self->capacity - self->size >= elements;
    }
#else
    static inline bool mtdp_buffer_pool_has_space_for(const mtdp_buffer_pool* self, size_t elements) {
        return MTDP_BUFFER_POOL_STATIC_SIZE - self->size >= elements;
    }
#endif


static inline bool mtdp_buffer_pool_preventive_realloc(mtdp_buffer_pool* self, size_t elements) {
    if(!mtdp_buffer_pool_has_space_for(self, elements)) {
#if MTDP_BUFFER_POOL_STATIC_SIZE
        return false;
#else
        static const size_t MIN_ALLOC_CAP = 16;
        size_t cap = (size_t) round(pow(2, ceil(log2((double) self->size + elements))));
        return mtdp_buffer_pool_realloc(self, cap > MIN_ALLOC_CAP ? cap : MIN_ALLOC_CAP);
#endif
    }
    return true;
}

void mtdp_buffer_pool_init(mtdp_buffer_pool* self)
{
#if MTDP_BUFFER_POOL_STATIC_SIZE
    self->size = 0;
#else
    self->size = self->capacity = 0;
#endif
}

void mtdp_buffer_pool_destroy(mtdp_buffer_pool* self)
{
#if MTDP_BUFFER_POOL_STATIC_SIZE
    (void) self;
#else
    if(self->capacity) {
        free(self->buffers);
        self->capacity = 0;
    }
#endif
}

bool mtdp_buffer_pool_push_back(mtdp_buffer_pool* self, const mtdp_buffer e)
{
    if(!mtdp_buffer_pool_preventive_realloc(self, 1)) {
        return false;
    }
    self->buffers[self->size++] = e;
    return true;
}

mtdp_buffer mtdp_buffer_pool_pop_back(mtdp_buffer_pool* self)
{
    if(self->size) {
        return self->buffers[--self->size];
    }
    return NULL;
}

size_t mtdp_buffer_pool_size(const mtdp_buffer_pool* self)
{
    return self->size;
}

bool mtdp_buffer_pool_resize(mtdp_buffer_pool *self, size_t size)
{
    bool ret = true;

#if MTDP_BUFFER_POOL_STATIC_SIZE
    size_t capacity = MTDP_BUFFER_POOL_STATIC_SIZE;
#else
    size_t capacity = self->capacity;
#endif
    signed long delta = (signed long) size - (signed long) capacity;
    if(delta < 0) {
        self->size += delta;
    } else if(delta > 0) {
#if MTDP_BUFFER_POOL_STATIC_SIZE
        return false;
#else
        ret = mtdp_buffer_pool_realloc(self, size);
        if(ret) {
            self->size += delta;
        }
#endif
    }
    return ret;
}

#undef MTDP_MEMORY_ACCESS
#define MTDP_MEMORY_ACCESS static inline

#if !MTDP_BUFFER_FIFO_BLOCKS
#   if MTDP_BUFFER_FIFO_BLOCK_STATIC_INSTANCES
#       if MTDP_BUFFER_FIFO_BLOCK_STATIC_INSTANCES > 0
            MTDP_DEFINE_STATIC_INSTANCES(mtdp_buffer_fifo_block, mtdp_static_buffer_fifo_blocks, MTDP_BUFFER_FIFO_BLOCK_STATIC_INSTANCES)
#       else
            MTDP_DEFINE_STATIC_INSTANCE(mtdp_buffer_fifo_block, mtdp_static_buffer_fifo_block)
#       endif
#   else
#       include <stdlib.h>
        MTDP_DEFINE_DYNAMIC_INSTANCE(mtdp_buffer_fifo_block)
#   endif
#endif

#undef MTDP_MEMORY_ACCESS

#if !MTDP_BUFFER_FIFO_BLOCKS
static inline bool mtdp_buffer_fifo_blocks_init(mtdp_buffer_fifo_blocks* blocks)
{
#   if !MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE
    mtdp_buffer_aggregate* vec = (mtdp_buffer_aggregate*) malloc(sizeof(mtdp_buffer_aggregate));
    if(!vec) {
        blocks->size = blocks->capacity = 0;
        blocks->blocks = NULL;
        return false;
    }
#   endif
    mtdp_buffer_aggregate tmp = (mtdp_buffer_aggregate) mtdp_buffer_fifo_block_alloc();
    if(tmp) {
#   if MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE
        blocks[MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE - 1] = tmp;
#   else
        blocks->blocks = vec;
        vec[0] = tmp;

        blocks->size = 1;
        blocks->capacity = 1;
#   endif
    }
#   if !MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE
    else {
        free(vec);
    }
#   endif
    return !!tmp;
}
#endif

bool mtdp_buffer_fifo_init(mtdp_buffer_fifo* self)
{
    self->size = 0;
#if MTDP_BUFFER_FIFO_BLOCKS
    self->first_element_offset = MTDP_BUFFER_FIFO_BLOCKS;
#else
    if(!mtdp_buffer_fifo_blocks_init(&self->blocks)) {
        return false;
    }
    self->first_element_offset = MTDP_BUFFER_FIFO_BLOCK_SIZE;
#endif
    return true;
}

void mtdp_buffer_fifo_destroy(mtdp_buffer_fifo* self)
{    
#if !MTDP_BUFFER_FIFO_BLOCKS
#   if !MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE
    for(size_t i = 0; i != self->blocks.size; ++i) {
        mtdp_buffer_fifo_block_dealloc((mtdp_buffer_fifo_block*) self->blocks.blocks[self->blocks.capacity - i - 1]);
    }
    self->blocks.size = 0;
    self->blocks.capacity = 0;
    free(self->blocks.blocks);
#   else
    size_t sz = MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE;
    for(; sz--; ) {
        mtdp_buffer_fifo_block_dealloc(self->blocks[sz]);
    }
#   endif
#else
    (void) self;
#endif
}

static inline void mtdp_buffer_fifo_rightshift_elements(mtdp_buffer_fifo* self)
{
#if MTDP_BUFFER_FIFO_BLOCKS
    /* Let's take advantage of the contiguous memory layout. */
    mtdp_buffer* buf = &self->blocks[0][0]; 
    for(size_t i = 1; i <= self->size; ++i) {
        buf[MTDP_BUFFER_FIFO_BLOCK_SIZE*MTDP_BUFFER_FIFO_BLOCKS - i] = buf[self->first_element_offset + self->size - i];
    }
    self->first_element_offset = MTDP_BUFFER_FIFO_BLOCK_SIZE*MTDP_BUFFER_FIFO_BLOCKS - self->size;
#else
    size_t capacity;
    size_t buffer_capacity;
    size_t delta;
    size_t rightmost_starting_block;
    size_t rightmost_starting;
    size_t front_starting_block;
    size_t front_starting;
    size_t front_starting_index;
#   if MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE
    capacity = MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE;
#   else
    capacity = self->blocks.capacity;
#   endif

    buffer_capacity = self->blocks.size*MTDP_BUFFER_FIFO_BLOCK_SIZE;
    delta = buffer_capacity - (self->first_element_offset + self->size);
    capacity--; /* In the loop we should use capacity - 1 everywhere. I'll reduce it once. */
    for(size_t i = 0; i < self->size; ++i) {
        rightmost_starting_block = capacity - i/MTDP_BUFFER_FIFO_BLOCK_SIZE;
        rightmost_starting = (MTDP_BUFFER_FIFO_BLOCK_SIZE - 1) - i % MTDP_BUFFER_FIFO_BLOCK_SIZE;
        front_starting_index = i + delta;
        front_starting_block = capacity - front_starting_index/MTDP_BUFFER_FIFO_BLOCK_SIZE;
        front_starting = (MTDP_BUFFER_FIFO_BLOCK_SIZE - 1) - front_starting_index % MTDP_BUFFER_FIFO_BLOCK_SIZE;
        self->blocks.blocks[rightmost_starting_block][rightmost_starting] = self->blocks.blocks[front_starting_block][front_starting];
    }
    self->first_element_offset += delta; 
#endif
}

#if defined(__GNUC__) || defined(__clang__)
#   define likely(expr)    (__builtin_expect(!!(expr), 1))
#   define unlikely(expr)  (__builtin_expect(!!(expr), 0))
#else
#   define likely(expr) (expr)
#   define unlikely(expr) (expr)
#endif

#if !MTDP_BUFFER_FIFO_BLOCKS
    static inline bool mtdp_buffer_fifo_try_reserve_a_block(mtdp_buffer_fifo* deque) {
#   if MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE
        return deque->blocks.size == MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE;
#   else
        mtdp_buffer_aggregate* tmp = deque->blocks.blocks;
        size_t cap;

        if(deque->blocks.capacity == deque->blocks.size) {
            cap = (size_t) round(pow(2, 1 + ceil(log2((double) deque->blocks.capacity))));
            tmp = (mtdp_buffer_aggregate*) realloc(deque->blocks.blocks, cap*sizeof(mtdp_buffer_aggregate));
            if(tmp) {
                deque->blocks.blocks = tmp;
                deque->blocks.capacity = cap;
                /* Blocks' rightshift. */
                for(size_t i = 1; i <= deque->blocks.size; i++) {
                    deque->blocks.blocks[deque->blocks.capacity - i] = deque->blocks.blocks[deque->blocks.size - i];
                }
            }
        }
        return !!tmp;
#   endif
    }
#endif

static inline mtdp_buffer* mtdp_buffer_fifo_shift_back_left(mtdp_buffer_fifo* self)
{
    mtdp_buffer* tmp = NULL;
#if MTDP_BUFFER_FIFO_BLOCKS
    mtdp_buffer* first = &self->blocks[0][0]; /* Contiguous memory layout */
    if(!self->first_element_offset && self->size != MTDP_BUFFER_FIFO_BLOCK_SIZE*MTDP_BUFFER_FIFO_BLOCKS) {
        mtdp_buffer_fifo_rightshift_elements(self);
    }
    if(self->first_element_offset) {
        tmp = &first[--self->first_element_offset];
    }
#else
#   if MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE
    size_t capacity = MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE;
#   else
    size_t capacity = self->blocks.capacity;
#   endif
    /* 
        This is probably the most complex function in the library. I usually don't comment business logic leaving 
        the code to talk by itself but there were 4 `goto`s to 2 different labels in it and I needed to structure
        it better, for professional pride. So these comments are a way to mentally formulate the function.

        There are different corner cases to check and a single strategy to mitigate allocation errors,
        i.e. rightshifting the values in order to leave space on the left.
        Shifting is both a strategy to execute when the deque is almost empty (according to the filling ratio),
        and a backup strategy to attempt when in a corner case and memory allocation fails. 
        When used as a backup strategy, shifting should only be attempted, since if the vector is already full 
        there's nothing the shifting can do.
        
        This function now should be deterministic enough to not require anyone to touch it anymore,
        but never say never. Come on, I saw things much worse than this, at least it's single threaded.
            - DT

        PS It was actually very bad when implemented without thinking on it. Now it looks quite comfy, it's
        just three `if`s.
    */
    mtdp_buffer_aggregate tmp_block;
    bool free_space_on_the_left = self->first_element_offset != 0;

    if(
        unlikely(!free_space_on_the_left
        /* 
            Here we first evaluate if we want to allocate more space looking at the filling ratio.
            We need more memory if the filling ratio is higher than the threshold (i.e. too many items).
            if that's the case then we try to reserve space in the block vector to allocate a new block.
        */
            && self->size >= self->blocks.size*MTDP_BUFFER_FIFO_BLOCK_SIZE*MTDP_BUFFER_FIFO_SHIFT_FILLING_RATIO
            && mtdp_buffer_fifo_try_reserve_a_block(self)
    )) {
        tmp_block = (mtdp_buffer_aggregate) mtdp_buffer_fifo_block_alloc();
        if(tmp_block) {
#   if !MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE
            capacity = self->blocks.capacity;
#   endif
            /* Allocation succeded. We have now (a lot of) space on the left */
            self->blocks.blocks[capacity - ++self->blocks.size] = tmp_block;
            self->first_element_offset = MTDP_BUFFER_FIFO_BLOCK_SIZE;
            free_space_on_the_left = true;
        }
    }

    /*
        We will enter in this branch if we wanted to shift to avoid allocating more memory,
        or if we are obliged because a memory allocation failed, either on the block vector or on a block.
    */
    if(!free_space_on_the_left && self->size != self->blocks.size*MTDP_BUFFER_FIFO_BLOCK_SIZE) {
        mtdp_buffer_fifo_rightshift_elements(self);
        free_space_on_the_left = self->first_element_offset != 0;
    }

    /*
        Notice that if the buffer was full, no shifting was possible and the function will return NULL,
        as expected and handled by the caller.
    */
    if(free_space_on_the_left) {
        self->size++;
        tmp_block = self->blocks.blocks[capacity - self->blocks.size + --self->first_element_offset/MTDP_BUFFER_FIFO_BLOCK_SIZE];
        tmp = &tmp_block[self->first_element_offset % MTDP_BUFFER_FIFO_BLOCK_SIZE];
    }
#endif
    return tmp;
}

bool mtdp_buffer_fifo_push_back(mtdp_buffer_fifo *self, const mtdp_buffer e)
{
    mtdp_buffer* back = mtdp_buffer_fifo_shift_back_left(self);
    if(!back) {
        return false;
    }
    *back = e;
    return true;
}

static inline mtdp_buffer* mtdp_buffer_fifo_shift_front_left(mtdp_buffer_fifo* self) {
    mtdp_buffer* tmp = NULL;
#if MTDP_BUFFER_FIFO_BLOCKS
    mtdp_buffer* first = &self->blocks[0][0]; /* Contiguous memory layout */
    if(self->size) {
        tmp = &first[MTDP_BUFFER_FIFO_BLOCKS*MTDP_BUFFER_FIFO_BLOCK_SIZE - (self->first_element_offset + self->size--) - 1];
        if(!self->size) {
            self->first_element_offset = MTDP_BUFFER_FIFO_BLOCKS*MTDP_BUFFER_FIFO_BLOCK_SIZE - 1;
        }
    }
#else
    mtdp_buffer_aggregate last_block;
    size_t capacity, front_index_in_block;

#   if MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE
    capacity = MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE;
#   else
    capacity = self->blocks.capacity;
#   endif
    if(self->size) {
        front_index_in_block = self->blocks.size*MTDP_BUFFER_FIFO_BLOCK_SIZE - (self->size + self->first_element_offset);
        last_block = self->blocks.blocks[capacity - 1];
        tmp = &last_block[MTDP_BUFFER_FIFO_BLOCK_SIZE - front_index_in_block - 1];
        self->size--;
        if(front_index_in_block == MTDP_BUFFER_FIFO_BLOCK_SIZE - 1 && self->blocks.size != 1) {
            /*
                We rightshift the blocks, to expose the first usable block to the front.
                This should reduce memory allocations but it induces a O(n/BLOCK_SIZE) operation in the 
                100./BLOCK_SIZE percent of cases.
                REVIEW Profile some tests with and without the blocks rightshift and understand
                if it induces an optimization overall. I think it does (even because once
                memory is finished on the left, an O(n) rightshift is performed when pushing an element
                if under the filling ratio threshold).
                    - DT
            */
            size_t i;
            for(i = 1; i <= (self->first_element_offset%MTDP_BUFFER_FIFO_BLOCK_SIZE + self->size)/MTDP_BUFFER_FIFO_BLOCK_SIZE; ++i) {
                self->blocks.blocks[capacity - i] = self->blocks.blocks[capacity - i - 1];
            }
            self->blocks.blocks[capacity - i] = last_block;
            self->first_element_offset += MTDP_BUFFER_FIFO_BLOCK_SIZE;
        }
    }
#endif
    return tmp;
}

bool mtdp_buffer_fifo_pop_front(mtdp_buffer_fifo* self, mtdp_buffer* ret)
{
    mtdp_buffer* back = mtdp_buffer_fifo_shift_front_left(self);
    if(!back) {
        return false;
    }
    *ret = *back;
    return true;
}

size_t mtdp_buffer_fifo_size(const mtdp_buffer_fifo* self)
{
    return self->size;
}
