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

#ifndef MTDP_IMPL_SOURCE_H
#define MTDP_IMPL_SOURCE_H

#include "futex.h"
#include "mtdp/pipe.h"
#include "mtdp/source.h"
#include "worker.h"

#include <stdbool.h>

typedef struct {
    mtdp_source_context context;
    mtdp_worker         worker;
    mtdp_source         user_data;
    mtdp_pipe*          output_pipe;
    mtdp_futex          done;
    bool                initialized;
} mtdp_source_impl;

void mtdp_source_create_thread(mtdp_source_impl*);
void mtdp_source_destroy(mtdp_source_impl*);
void mtdp_source_configure(mtdp_source_impl*, mtdp_pipe* output_pipe);

#endif