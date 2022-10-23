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

#ifndef MTDP_ATOMIC_H
#define MTDP_ATOMIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#  include <windows.h>
#  include <winnt.h>
#  define atomic_bool                   volatile uint32_t
#  define atomic_uchar                  volatile uint32_t
#  define atomic_uint32_t               volatile uint32_t
#  define atomic_flag                   volatile uint32_t
#  define atomic_load(PTR)              (*PTR)
#  define atomic_store(PTR, VAL)        (*(PTR) = (VAL))
#  define atomic_fetch_or(PTR, VAL)     InterlockedOr((PTR), (VAL))
#  define atomic_fetch_and(PTR, VAL)    InterlockedAnd((PTR), (VAL))
#  define atomic_flag_test_and_set(PTR) InterlockedCompareExchange((PTR), 1, 0)
#  define ATOMIC_FLAG_INIT                                                                                                       \
    {                                                                                                                            \
      0                                                                                                                          \
    }
#elif __unix__
#  include <stdatomic.h>
#  define atomic_uint32_t _Atomic(uint32_t)
#else
#  error atomic not implemented on this platform
#endif

#endif