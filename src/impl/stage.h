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

#ifndef MTDP_IMPL_STAGE_H
#define MTDP_IMPL_STAGE_H

#include <stdbool.h>

#include "mtdp/stage.h"
#include "mtdp/pipe.h"
#include "worker.h"
#include "futex.h"

typedef struct {
    mtdp_stage_context context;
    mtdp_worker worker;
    mtdp_stage* user_data;
    mtdp_pipe* input_pipe;
    mtdp_pipe* output_pipe;
    mtdp_futex done;
    bool initialized;
} mtdp_stage_impl;

void mtdp_stage_create_thread(mtdp_stage_impl*);
void mtdp_stage_destroy(mtdp_stage_impl*);
void mtdp_stage_configure(mtdp_stage_impl*, mtdp_pipe* input_pipe, mtdp_pipe* output_pipe, mtdp_stage* user_data);

#if MTDP_STAGE_VECTOR_STATIC_SIZE
    typedef mtdp_stage mtdp_stage_vector[MTDP_STAGE_VECTOR_STATIC_SIZE];
#else
#   include <stdlib.h>
    typedef mtdp_stage* mtdp_stage_vector;
#endif

bool mtdp_stage_vector_resize(mtdp_stage_vector*, size_t n);
void mtdp_stage_vector_destroy(mtdp_stage_vector*);

#if MTDP_STAGE_IMPL_VECTOR_STATIC_SIZE
    typedef mtdp_stage_impl mtdp_stage_impl_vector[MTDP_STAGE_IMPL_VECTOR_STATIC_SIZE];
#else
#   include <stdlib.h>
    typedef mtdp_stage_impl* mtdp_stage_impl_vector;
#endif

bool mtdp_stage_impl_vector_resize(mtdp_stage_impl_vector*, size_t n);
void mtdp_stage_impl_vector_destroy(mtdp_stage_impl_vector*);

#endif