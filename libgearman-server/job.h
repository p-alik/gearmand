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

#pragma once

#include <libgearman-server/struct/job.h>

/** @addtogroup gearman_server_job Job Declarations @ingroup gearman_server
 *
 * This is a low level interface for gearman server jobs. This is used
 * internally by the server interface, so you probably want to look there first.
 *
 * @{
 */


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add a new job to a server instance.
 */
GEARMAN_API
gearman_server_job_st *
gearman_server_job_add(gearman_server_st *server,
                       const char *function_name, size_t function_name_size,
                       const char *unique, size_t unique_size,
                       const void *data, size_t data_size,
                       gearman_job_priority_t priority,
                       gearman_server_client_st *server_client,
                       gearmand_error_t *ret_ptr,
                       int64_t when);

GEARMAN_API
gearman_server_job_st *
gearman_server_job_add_reducer(gearman_server_st *server,
                               const char *function_name, size_t function_name_size, 
                               const char *unique, size_t unique_size, 
                               const char *reducer, size_t reducer_name_size, 
                               const void *data, size_t data_size,
                               gearman_job_priority_t priority,
                               gearman_server_client_st *server_client,
                               gearmand_error_t *ret_ptr,
                               int64_t when);



/**
 * Initialize a server job structure.
 */
GEARMAN_API
gearman_server_job_st *
gearman_server_job_create(gearman_server_st *server);

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
                                              const size_t job_handle_length,
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
gearmand_error_t gearman_server_job_queue(gearman_server_job_st *server_job);

uint32_t _server_job_hash(const char *key, size_t key_size);

void *_proc(void *data);

void _server_con_worker_list_append(gearman_server_worker_st *list,
                                    gearman_server_worker_st *worker);

gearman_server_job_st *gearman_server_job_get_by_unique(gearman_server_st *server,
                                                        const char *unique,
                                                        const size_t unique_length,
                                                        gearman_server_con_st *worker_con);

/** @} */

void destroy_gearman_server_job_st(gearman_server_job_st *);

#ifdef __cplusplus
}
#endif
