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
along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "mtdp/errno.h"
#include "impl/errno.h"

#if defined(_WIN32)
static thread_local enum mtdp_error mtdp_errno_location = MTDP_OK;
#else
static _Thread_local enum mtdp_error mtdp_errno_location = MTDP_OK;
#endif

enum mtdp_error* mtdp_errno_ptr_mutable()
{
    return &mtdp_errno_location;
}

const enum mtdp_error* mtdp_errno_ptr()
{
    return &mtdp_errno_location;
}