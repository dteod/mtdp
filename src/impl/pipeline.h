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

#ifndef MTDP_IMPL_PIPELINE_H
#define MTDP_IMPL_PIPELINE_H

#include "atomic.h"
#include "impl/errno.h"
#include "impl/pipe.h"
#include "impl/sink.h"
#include "impl/source.h"
#include "impl/stage.h"
#include "memory.h"
#include "mtdp/pipeline.h"
#include "thread.h"

#include <stddef.h>
#include <stdint.h>

struct s_mtdp_pipeline {
    mtdp_source_impl       source_impl;
    mtdp_stage_impl_vector stage_impls;
    mtdp_sink_impl         sink_impl;

    mtdp_stage_vector stages;
    mtdp_pipe_vector  pipes;

    size_t          n_stages;
    bool            enabled, active;
    atomic_uint32_t destroying;
};

MTDP_DECLARE_INSTANCE(mtdp_pipeline)

#endif
