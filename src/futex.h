/* Copyright (C) 2021-2022 Domenico Teodonio

mtdp is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

mtdp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see
<http://www.gnu.org/licenses/>.  */

#ifndef MTDP_FUTEX_H
#define MTDP_FUTEX_H

#include <stdint.h>

typedef _Atomic(uint32_t) mtdp_futex;

void mtdp_futex_wait(mtdp_futex*, uint32_t expected);
void mtdp_futex_notify_one(mtdp_futex*);
void mtdp_futex_notify_all(mtdp_futex*);

#endif