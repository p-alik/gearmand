/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman Server Declarations
 */

#ifndef __GEARMAN_SERVER_H__
#define __GEARMAN_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server Gearman Server Declarations
 * This is the interface gearman servers should use.
 * @{
 */

struct gearman_server_st
{
  struct {
    bool allocated;
  } options;
  struct {
    bool round_robin;
    bool threaded;
  } flags;
  struct {
    bool queue_startup;
  } state;
  bool shutdown;
  bool shutdown_graceful;
  bool proc_wakeup;
  bool proc_shutdown;
  uint8_t job_retries;
  uint8_t worker_wakeup;
  uint32_t job_handle_count;
  uint32_t thread_count;
  uint32_t function_count;
  uint32_t job_count;
  uint32_t unique_count;
  uint32_t free_packet_count;
  uint32_t free_job_count;
  uint32_t free_client_count;
  uint32_t free_worker_count;
  gearman_universal_st *gearman;
  gearman_server_thread_st *thread_list;
  gearman_server_function_st *function_list;
  gearman_server_packet_st *free_packet_list;
  gearman_server_job_st *free_job_list;
  gearman_server_client_st *free_client_list;
  gearman_server_worker_st *free_worker_list;
  gearman_log_fn *log_fn;
  void *log_context;
  void *queue_context;
  gearman_queue_add_fn *queue_add_fn;
  gearman_queue_flush_fn *queue_flush_fn;
  gearman_queue_done_fn *queue_done_fn;
  gearman_queue_replay_fn *queue_replay_fn;
  gearman_universal_st gearman_universal_static;
  pthread_mutex_t proc_lock;
  pthread_cond_t proc_cond;
  pthread_t proc_id;
  char job_handle_prefix[GEARMAN_JOB_HANDLE_SIZE];
  gearman_server_job_st *job_hash[GEARMAN_JOB_HASH_SIZE];
  gearman_server_job_st *unique_hash[GEARMAN_JOB_HASH_SIZE];
};

/**
 * Initialize a server structure. This cannot fail if the caller supplies a
 * server structure.
 * @param server Caller allocated server structure, or NULL to allocate one.
 * @return Pointer to an allocated server structure if server parameter was
 *         NULL, or the server parameter pointer if it was not NULL.
 */
GEARMAN_API
gearman_server_st *gearman_server_create(gearman_server_st *server);

/**
 * Free resources used by a server structure.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 */
GEARMAN_API
void gearman_server_free(gearman_server_st *server);

/**
 * Set maximum job retry count.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @param job_retries Number of job attempts.
 */
GEARMAN_API
void gearman_server_set_job_retries(gearman_server_st *server,
                                    uint8_t job_retries);

/**
 * Set maximum number of workers to wake up per job.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @param worker_wakeup Number of workers to wake up.
 */
GEARMAN_API
void gearman_server_set_worker_wakeup(gearman_server_st *server,
                                      uint8_t worker_wakeup);

/**
 * Set logging callback for server instance.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @param function Function to call when there is a logging message.
 * @param context Argument to pass into the log callback function.
 * @param verbose Verbosity level.
 */
GEARMAN_API
void gearman_server_set_log_fn(gearman_server_st *server,
                               gearman_log_fn *function,
                               void *context, gearman_verbose_t verbose);

/**
 * Process commands for a connection.
 * @param server_con Server connection that has a packet to process.
 * @param packet The packet that needs processing.
 * @return Standard gearman return value.
 */
GEARMAN_API
gearman_return_t gearman_server_run_command(gearman_server_con_st *server_con,
                                            gearman_packet_st *packet);

/**
 * Tell server that it should enter a graceful shutdown state.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @return Standard gearman return value. This will return GEARMAN_SHUTDOWN if
 *         the server is ready to shutdown now.
 */
GEARMAN_API
gearman_return_t gearman_server_shutdown_graceful(gearman_server_st *server);

/**
 * Replay the persistent queue to load all unfinshed jobs into the server. This
 * should only be run at startup.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @return Standard gearman return value. This will return GEARMAN_SHUTDOWN if
 *         the server is ready to shutdown now.
 */
GEARMAN_API
gearman_return_t gearman_server_queue_replay(gearman_server_st *server);

/**
 * Get persistent queue context.
 */
GEARMAN_API
void *gearman_server_queue_context(const gearman_server_st *server);

/**
 * Set persistent queue context that will be passed back to all queue callback
 * functions.
 */
GEARMAN_API
void gearman_server_set_queue_context(gearman_server_st *server,
                                      void *context);

/**
 * Set function to call when jobs need to be stored in the persistent queue.
 */
GEARMAN_API
void gearman_server_set_queue_add_fn(gearman_server_st *server,
                                     gearman_queue_add_fn *function);

/**
 * Set function to call when the persistent queue should be flushed to disk.
 */
GEARMAN_API
void gearman_server_set_queue_flush_fn(gearman_server_st *server,
                                       gearman_queue_flush_fn *function);

/**
 * Set function to call when a job should be removed from the persistent queue.
 */
GEARMAN_API
void gearman_server_set_queue_done_fn(gearman_server_st *server,
                                      gearman_queue_done_fn *function);

/**
 * Set function to call when jobs in the persistent queue should be replayed
 * after a restart.
 */
GEARMAN_API
void gearman_server_set_queue_replay_fn(gearman_server_st *server,
                                        gearman_queue_replay_fn *function);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_H__ */
