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

#ifndef __GEARMAN_SERVER_THREAD_H__
#define __GEARMAN_SERVER_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server_thread Thread Declarations
 * @ingroup gearman_server
 *
 * This is the interface gearman servers should use for creating threads.
 *
 * @{
 */

struct gearman_server_thread_st
{
  struct {
    bool allocated;
  } options;
  uint32_t con_count;
  uint32_t io_count;
  uint32_t proc_count;
  uint32_t free_con_count;
  uint32_t free_packet_count;
  gearman_universal_st *gearman;
  gearman_server_st *server;
  gearman_server_thread_st *next;
  gearman_server_thread_st *prev;
  gearman_log_fn *log_fn;
  void *log_context;
  gearman_server_thread_run_fn *run_fn;
  void *run_fn_arg;
  gearman_server_con_st *con_list;
  gearman_server_con_st *io_list;
  gearman_server_con_st *proc_list;
  gearman_server_con_st *free_con_list;
  gearman_server_packet_st *free_packet_list;
  gearman_universal_st gearman_universal_static;
  pthread_mutex_t lock;
};

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
gearman_server_thread_st *
gearman_server_thread_create(gearman_server_st *server,
                             gearman_server_thread_st *thread);

/**
 * Free resources used by a thread structure.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 */
GEARMAN_API
void gearman_server_thread_free(gearman_server_thread_st *thread);

/**
 * Return an error string for the last error encountered.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 * @return Pointer to static buffer in library that holds an error string.
 */
GEARMAN_API
const char *gearman_server_thread_error(gearman_server_thread_st *thread);

/**
 * Value of errno in the case of a GEARMAN_ERRNO return value.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 * @return An errno value as defined in your system errno.h file.
 */
GEARMAN_API
int gearman_server_thread_errno(gearman_server_thread_st *thread);

/**
 * Set custom I/O event watch callback.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 * @param event_watch Function to be called when events need to be watched.
 * @param event_watch_arg Argument to pass along to event_watch.
 */
GEARMAN_API
void gearman_server_thread_set_event_watch(gearman_server_thread_st *thread,
                                           gearman_event_watch_fn *event_watch,
                                           void *event_watch_arg);

/**
 * Set logging callback for server thread instance.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
 * @param function Function to call when there is a logging message.
 * @param context Argument to pass into the log callback function.
 * @param verbose Verbosity level.
 */
GEARMAN_API
void gearman_server_thread_set_log_fn(gearman_server_thread_st *thread,
                                      gearman_log_fn *function,
                                      void *context,
                                      gearman_verbose_t verbose);

/**
 * Set thread run callback.
 * @param thread Thread structure previously initialized with
 *        gearman_server_thread_create.
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
 *        gearman_server_thread_create.
 * @param ret_ptr Pointer to hold a standard gearman return value.
 * @return On error, the server connection that encountered the error.
 */
GEARMAN_API
gearman_server_con_st *
gearman_server_thread_run(gearman_server_thread_st *thread,
                          gearman_return_t *ret_ptr);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_THREAD_H__ */
