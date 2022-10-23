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

#ifndef MTDP_IMPL_SINK_H
#define MTDP_IMPL_SINK_H

#include "futex.h"
#include "mtdp/pipe.h"
#include "mtdp/sink.h"
#include "worker.h"

#include <stdbool.h>

typedef struct {
    mtdp_sink_context context;
    mtdp_worker       worker;
    mtdp_sink         user_data;
    mtdp_pipe*        input_pipe;
    mtdp_futex        done;
    bool              initialized;
} mtdp_sink_impl;

void mtdp_sink_create_thread(mtdp_sink_impl*);
void mtdp_sink_destroy(mtdp_sink_impl*);
void mtdp_sink_configure(mtdp_sink_impl*, mtdp_pipe* input_pipe);

#endif