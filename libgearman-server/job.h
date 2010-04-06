/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Job Declarations
 */

#ifndef __GEARMAN_SERVER_JOB_H__
#define __GEARMAN_SERVER_JOB_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server_job Job Declarations
 * @ingroup gearman_server
 *
 * This is a low level interface for gearman server jobs. This is used
 * internally by the server interface, so you probably want to look there first.
 *
 * @{
 */

struct gearman_server_job_st
{
  uint8_t retries;
  struct {
    bool allocated;
  } options;
  gearman_job_priority_t priority;
  bool ignore_job;
  bool job_queued;
  uint32_t job_handle_key;
  uint32_t unique_key;
  uint32_t client_count;
  uint32_t numerator;
  uint32_t denominator;
  size_t data_size;
  gearman_server_st *server;
  gearman_server_job_st *next;
  gearman_server_job_st *prev;
  gearman_server_job_st *unique_next;
  gearman_server_job_st *unique_prev;
  gearman_server_job_st *worker_next;
  gearman_server_job_st *worker_prev;
  gearman_server_function_st *function;
  gearman_server_job_st *function_next;
  const void *data;
  gearman_server_client_st *client_list;
  gearman_server_worker_st *worker;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  char unique[GEARMAN_UNIQUE_SIZE];
};

/**
 * Add a new job to a server instance.
 */
GEARMAN_API
gearman_server_job_st *
gearman_server_job_add(gearman_server_st *server, const char *function_name,
                       size_t function_name_size, const char *unique,
                       size_t unique_size, const void *data, size_t data_size,
                       gearman_job_priority_t priority,
                       gearman_server_client_st *server_client,
                       gearman_return_t *ret_ptr);

/**
 * Initialize a server job structure.
 */
GEARMAN_API
gearman_server_job_st *
gearman_server_job_create(gearman_server_st *server,
                          gearman_server_job_st *server_job);

/**
 * Free a server job structure.
 */
GEARMAN_API
void gearman_server_job_free(gearman_server_job_st *server_job);

/**
 * Get a server job structure from the job handle.
 */
GEARMAN_API
gearman_server_job_st *gearman_server_job_get(gearman_server_st *server,
                                              const char *job_handle,
                                              gearman_server_con_st *worker_con);

/**
 * See if there are any jobs to be run for the server worker connection.
 */
GEARMAN_API
gearman_server_job_st *
gearman_server_job_peek(gearman_server_con_st *server_con);

/**
 * Start running a job for the server worker connection.
 */
GEARMAN_API
gearman_server_job_st *
gearman_server_job_take(gearman_server_con_st *server_con);

/**
 * Queue a job to be run.
 */
GEARMAN_API
gearman_return_t gearman_server_job_queue(gearman_server_job_st *server_job);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_JOB_H__ */
