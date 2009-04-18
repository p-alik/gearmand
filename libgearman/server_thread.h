/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server Thread Declarations
 */

#ifndef __GEARMAN_SERVER_THREAD_H__
#define __GEARMAN_SERVER_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server_thread Server Thread Handling
 * This is the interface gearman servers should use for creating threads.
 * @{
 */

/**
 * Initialize a thread structure. This cannot fail if the caller supplies a
 * thread structure.
 * @param thread Caller allocated thread structure, or NULL to allocate one.
 * @return Pointer to an allocated thread structure if thread parameter was
 *         NULL, or the thread parameter pointer if it was not NULL.
 */
gearman_server_thread_st *
gearman_server_thread_create(gearman_server_st *server,
                             gearman_server_thread_st *thread);

/**
 * Free resources used by a thread structure.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 */
void gearman_server_thread_free(gearman_server_thread_st *thread);

/**
 * Return an error string for the last error encountered.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 * @return Pointer to static buffer in library that holds an error string.
 */
const char *gearman_server_thread_error(gearman_server_thread_st *thread);

/**
 * Value of errno in the case of a GEARMAN_ERRNO return value.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 * @return An errno value as defined in your system errno.h file.
 */
int gearman_server_thread_errno(gearman_server_thread_st *thread);

/**
 * Set custom I/O event watch callback.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 * @param event_watch Function to be called when events need to be watched.
 * @param event_watch_arg Argument to pass along to event_watch.
 */
void gearman_server_thread_set_event_watch(gearman_server_thread_st *thread,
                                           gearman_event_watch_fn *event_watch,
                                           void *event_watch_arg);

/**
 * Set logging callback for server thread instance.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 * @param log_fn Function to call when there is a logging message.
 * @param log_fn_arg Argument to pass into the log callback function.
 */
void gearman_server_thread_set_log(gearman_server_thread_st *thread,
                                   gearman_server_thread_log_fn log_fn,
                                   void *log_fn_arg);

/**
 * Set thread run callback.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 * @param run_fn Function to call when thread should be run.
 * @param run_arg Argument to pass along with run_fn.
 */
void gearman_server_thread_set_run(gearman_server_thread_st *thread,
                                   gearman_server_thread_run_fn *run_fn,
                                   void *run_arg);

/**
 * Process server thread connections.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 * @param ret_ptr Pointer to hold a standard gearman return value.
 * @return On error, the server connection that encountered the error.
 */
gearman_server_con_st *
gearman_server_thread_run(gearman_server_thread_st *thread,
                          gearman_return_t *ret_ptr);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_THREAD_H__ */
