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

#ifndef MTDP_ERRNO_H
#define MTDP_ERRNO_H

/**
 * @brief Enumeration describing possible causes of faults.
 */
enum mtdp_error {
    /** Successful return. */
    MTDP_OK,
    /** A memory allocation failed. */
    MTDP_NO_MEM,
    /** Requested operation an active pipeline */
    MTDP_ACTIVE,
    /** Requested to wait on/enabled/stop an enabled pipeline */
    MTDP_ENABLED,
    /** Requested to wait on/disable/start/stop a disabled pipeline */
    MTDP_NOT_ENABLED,
    /** Requested operation on something that is not a pipeline */
    MTDP_BAD_PTR,

    /*
        The following are errors that should not normally display
        under normal circumstances. These indicate failure at the
        C runtime implementation level.
    */
    /** Error on a thrd_* function call */
    MTDP_THRD_ERROR,
    /** Error on a mtx_* function call */
    MTDP_MTX_ERROR,
    /** Error on a cnd_* function call */
    MTDP_CND_ERROR,

};

/**
 * @brief Returns a pointer to the error code variable used by libmtdp.
 * 
 * @details The `mtdp_errno` convenience macro wraps this function to provide
 * a reference-like access, similar to errno.
 * 
 * @note mtdp_errno shall not be set by the user, it is internally set
 * by the library on every function call (even successful ones).
 * 
 * @return const enum mtdp_error* pointer to the mtdp_errno variable
 */
const enum mtdp_error* mtdp_errno_ptr();

/**
 * @brief Latest return status for mtdp functions.
 * 
 * @note The retrieved value is thread local.
 */
#define mtdp_errno (*mtdp_errno_ptr())

#endif
