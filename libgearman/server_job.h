/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server job declarations
 */

#ifndef __GEARMAN_SERVER_JOB_H__
#define __GEARMAN_SERVER_JOB_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server_job Server Job Handling
 * This is a low level interface for gearman server jobs. This is used
 * internally by the server interface, so you probably want to look there first.
 * @{
 */

/**
 * Add a new job to a server instance.
 */
gearman_server_job_st *gearman_server_job_add(gearman_server_st *server,
                                              const char *function_name,
                                              size_t function_name_size,
                                              const void *data,
                                              size_t data_size,
                                              gearman_server_con_st *server_con,
                                              bool high);

/**
 * Initialize a server job structure.
 */
gearman_server_job_st *gearman_server_job_create(gearman_server_st *server,
                                             gearman_server_job_st *server_job);

/**
 * Free a server job structure.
 */
void gearman_server_job_free(gearman_server_job_st *server_job);

/**
 * Get a server job structure from the job handle.
 */
gearman_server_job_st *gearman_server_job_get(gearman_server_st *server,
                                              gearman_job_handle_t job_handle);

/**
 * Get a new server job that the server connection is registered to run.
 */
gearman_server_job_st *gearman_server_job_get_new(gearman_server_con_st *server_con);

/**
 * Note that a job is being run by the given server connection.
 */
void gearman_server_job_run(gearman_server_job_st *server_job,
                            gearman_server_con_st *server_con);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_JOB_H__ */
