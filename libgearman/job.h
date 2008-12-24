/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Job declarations
 */

#ifndef __GEARMAN_JOB_H__
#define __GEARMAN_JOB_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_job Job Management
 * @ingroup gearman_worker
 * The job functions are used to manage jobs assigned to workers. It is most
 * commonly used with the worker interface.
 * @{
 */

/**
 * Initialize a job structure.
 */
gearman_job_st *gearman_job_create(gearman_st *gearman, gearman_job_st *job);

/**
 * Free a job structure.
 */
void gearman_job_free(gearman_job_st *job);

/**
 * Send data for a running job.
 */
gearman_return_t gearman_job_data(gearman_job_st *job, void *data,
                                  size_t data_size);

/**
 * Send status information for a running job.
 */
gearman_return_t gearman_job_status(gearman_job_st *job, uint32_t numerator,
                                    uint32_t denominator);

/**
 * Send result and complete status for a job.
 */
gearman_return_t gearman_job_complete(gearman_job_st *job, void *result,
                                      size_t result_size);

/**
 * Send fail status for a job.
 */
gearman_return_t gearman_job_fail(gearman_job_st *job);

/**
 * Get job handle.
 */
char *gearman_job_handle(gearman_job_st *job);

/**
 * Get the function name associated with a job.
 */
char *gearman_job_function_name(gearman_job_st *job);

/**
 * Get a pointer to the workload for a job.
 */
const void *gearman_job_workload(gearman_job_st *job);

/**
 * Get size of the workload for a job.
 */
size_t gearman_job_workload_size(gearman_job_st *job);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_JOB_H__ */
