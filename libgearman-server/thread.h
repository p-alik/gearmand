/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#pragma once

#include <libgearman-server/struct/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

/**
 * @addtogroup gearman_server_thread Thread Declarations
 * @ingroup gearman_server
 *
 * This is the interface gearman servers should use for creating threads.
 *
 * @{
 */

/**
 * Initialize a thread structure. This cannot fail if the caller supplies a
 * thread structure.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @param thread Caller allocated thread structure, or NULL to allocate one.
 * @return Pointer to an allocated thread structure if thread parameter was
 *         NULL, or the thread parameter pointer if it was not NULL.
 */
GEARMAN_API
bool gearman_server_thread_init(gearman_server_st *server,
                                gearman_server_thread_st *thread,
                                gearman_log_server_fn *function,
                                gearmand_thread_st *context,
                                gearmand_event_watch_fn *event_watch);

/**
 * Free resources used by a thread structure.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_init.
 */
GEARMAN_API
void gearman_server_thread_free(gearman_server_thread_st *thread);

/**
 * Set thread run callback.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_init.
 * @param run_fn Function to call when thread should be run.
 * @param run_arg Argument to pass along with run_fn.
 */
GEARMAN_API
void gearman_server_thread_set_run(gearman_server_thread_st *thread,
                                   gearman_server_thread_run_fn *run_fn,
                                   void *run_arg);

/**
 * Process server thread connections.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_init.
 * @param ret_ptr Pointer to hold a standard gearman return value.
 * @return On error, the server connection that encountered the error.
 */
GEARMAN_API
gearmand_con_st *
gearman_server_thread_run(gearman_server_thread_st *thread,
                          gearmand_error_t *ret_ptr);

/** @} */

#ifdef __cplusplus
}
#endif
