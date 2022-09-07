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


/**
 * @file 
 * 
 * @brief Umbrella header used to import libmtdp.
 * 
 * @details Avoid importing files directly from the subdirectory.
*/

#ifndef MTDP_H
#define MTDP_H

/** \cond */
#ifdef __cplusplus
#   include <cstdint>
#   include <cstddef>
extern "C" {
#else
#   include <stdint.h>
#   include <stddef.h>
#   include <stdbool.h>
#endif
/** \endcond */

#include "mtdp/errno.h"
#include "mtdp/pipeline.h"

#ifdef __cplusplus
}
#endif

#endif
