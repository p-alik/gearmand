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

#include <pthread.h>

/**
 * @addtogroup gearman_server Gearman Server Declarations
 * This is the interface gearman servers should use.
 * @{
 */

struct gearman_server_st
{
  struct {
    /*
      Sets the round-robin mode on the server object. RR will distribute work
      fairly among every function assigned to a worker, instead of draining
      each function before moving on to the next.
    */
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
  uint8_t job_retries; // Set maximum job retry count.
  uint8_t worker_wakeup; // Set maximum number of workers to wake up per job.
  uint32_t job_handle_count;
  uint32_t thread_count;
  uint32_t function_count;
  uint32_t job_count;
  uint32_t unique_count;
  uint32_t free_packet_count;
  uint32_t free_job_count;
  uint32_t free_client_count;
  uint32_t free_worker_count;
  gearman_server_thread_st *thread_list;
  gearman_server_function_st *function_list;
  gearman_server_packet_st *free_packet_list;
  gearman_server_job_st *free_job_list;
  gearman_server_client_st *free_client_list;
  gearman_server_worker_st *free_worker_list;
  struct {
    void *_context;
    gearman_queue_add_fn *_add_fn;
    gearman_queue_flush_fn *_flush_fn;
    gearman_queue_done_fn *_done_fn;
    gearman_queue_replay_fn *_replay_fn;
  } queue;
  pthread_mutex_t proc_lock;
  pthread_cond_t proc_cond;
  pthread_t proc_id;
  char job_handle_prefix[GEARMAND_JOB_HANDLE_SIZE];
  gearman_server_job_st *job_hash[GEARMAND_JOB_HASH_SIZE];
  gearman_server_job_st *unique_hash[GEARMAND_JOB_HASH_SIZE];
};


inline static void gearmand_set_round_robin(gearman_server_st *server, bool round_robin)
{
  server->flags.round_robin= round_robin;
}

/**
 * Process commands for a connection.
 * @param server_con Server connection that has a packet to process.
 * @param packet The packet that needs processing.
 * @return Standard gearman return value.
 */
GEARMAN_API
gearmand_error_t gearman_server_run_command(gearman_server_con_st *server_con,
                                            gearmand_packet_st *packet);

/**
 * Tell server that it should enter a graceful shutdown state.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @return Standard gearman return value. This will return GEARMAN_SHUTDOWN if
 *         the server is ready to shutdown now.
 */
GEARMAN_API
gearmand_error_t gearman_server_shutdown_graceful(gearman_server_st *server);

/**
 * Replay the persistent queue to load all unfinshed jobs into the server. This
 * should only be run at startup.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @return Standard gearman return value. This will return GEARMAN_SHUTDOWN if
 *         the server is ready to shutdown now.
 */
GEARMAN_API
gearmand_error_t gearman_server_queue_replay(gearman_server_st *server);

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
void gearman_server_set_queue(gearman_server_st *server,
                              void *context,
                              gearman_queue_add_fn *add,
                              gearman_queue_flush_fn *flush,
                              gearman_queue_done_fn *done,
                              gearman_queue_replay_fn *replay);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_H__ */
