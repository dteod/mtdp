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
 * @brief Header containing the mtdp_buffer alias.
 * @note Do not import this file in user code, use the mtdp.h umbrella header instead.
 */

#ifndef MTDP_BUFFER_H
#define MTDP_BUFFER_H

#ifndef MTDP_H
#   error do not #include <mtdp/buffer.h> directly, #include <mtdp.h> instead
#endif

/**
 * @brief Convenience alias used to abstract pointer syntax.
 */
typedef void* mtdp_buffer;

#endif
