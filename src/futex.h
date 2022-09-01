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
along with this program.  If not, see
<http://www.gnu.org/licenses/>.  */

#ifndef MTDP_FUTEX_H
#define MTDP_FUTEX_H

#include "atomic.h"

#define mtdp_futex atomic_uint32_t

/**
 * 
 * If you can get 2 billion threads to wait on a futex,
 * then I’m both impressed and disappointed.
 * Impressed that you were able to create 2 billion threads in the first place,
 * and disappointed that you have a futex so hot
 * that you managed to get 2 billion threads waiting on it.
 * You should fix that.
 *                  - Raymond Chen
 */

#ifdef __unix__
#   include <linux/futex.h>
#   include <sys/syscall.h>
#   include <unistd.h>
#   include <limits.h>
#   define mtdp_futex_wait(ftx, val) \
    while(atomic_load(ftx) == val && syscall(SYS_futex, (ftx), FUTEX_WAIT_PRIVATE, val, NULL) == 0) {}
#   define mtdp_futex_notify_one(ftx) (syscall(SYS_futex, (ftx), FUTEX_WAKE_PRIVATE, 1))
#   define mtdp_futex_notify_all(ftx) (syscall(SYS_futex, (ftx), FUTEX_WAKE_PRIVATE, INT_MAX))
#elif _WIN32
#   include <windows.h>
#   include <synchapi.h>
#   define mtdp_futex_wait(ftx, val) do { uint32_t val_ = val; do {} while((atomic_load(ftx) == val) && !WaitOnAddress((PVOID)ftx, &val_, sizeof(uint32_t), INFINITE)); } while(0)
#   define mtdp_futex_notify_one(ftx) WakeByAddressSingle((PVOID)ftx)
#   define mtdp_futex_notify_all(ftx) WakeByAddressAll((PVOID)ftx)
#else
#   error futex not implemented on this platform
#endif


#endif