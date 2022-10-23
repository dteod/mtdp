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

#ifndef MTDP_BELL_H
#define MTDP_BELL_H

#include "futex.h"
#include "thread.h"

#define mtdp_set_done(ftx)                                                                                                       \
  do {                                                                                                                           \
    atomic_store((ftx), 1);                                                                                                      \
    mtdp_futex_notify_all(ftx);                                                                                                  \
  } while(0)

#define mtdp_unset_done(ftx)                                                                                                     \
  do {                                                                                                                           \
    atomic_store((ftx), 0);                                                                                                      \
    mtdp_futex_notify_all(ftx);                                                                                                  \
  } while(0)

#endif
