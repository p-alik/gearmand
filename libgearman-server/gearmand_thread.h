/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Thread Declarations
 */

#pragma once

#include <pthread.h>

#include <libgearman-server/struct/gearmand_thread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearmand_thread Thread Declarations
 * @ingroup gearmand
 *
 * Thread handling for gearmand.
 *
 * @{
 */

/**
 * Create a new gearmand thread.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @return Standard gearman return value.
 */
GEARMAN_API
gearmand_error_t gearmand_thread_create(struct gearmand_st *gearmand);

/**
 * Free resources used by a thread.
 * @param thread Thread previously initialized with gearmand_thread_create.
 */
GEARMAN_API
void gearmand_thread_free(gearmand_thread_st *thread);

/**
 * Interrupt a running gearmand thread.
 * @param thread Thread structure previously initialized with
 * gearmand_thread_create.
 * @param wakeup Wakeup event to send to running thread.
 */
GEARMAN_API
void gearmand_thread_wakeup(gearmand_thread_st *thread,
                            gearmand_wakeup_t wakeup);

/**
 * Run the thread when there are events ready.
 * @param thread Thread structure previously initialized with
 * gearmand_thread_create.
 */
GEARMAN_API
void gearmand_thread_run(gearmand_thread_st *thread);

/** @} */

#ifdef __cplusplus
}
#endif
