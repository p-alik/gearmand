/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Task declarations
 */

#ifndef __GEARMAN_TASK_H__
#define __GEARMAN_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_task Task Management
 * @ingroup gearman_client
 * The task functions are used to manage tasks being run by clients. They are
 * most commonly used with the client interface.
 * @{
 */

/**
 * Initialize a task structure.
 */
gearman_task_st *gearman_task_create(gearman_st *gearman,
                                     gearman_task_st *task);

/**
 * Free a task structure.
 */
void gearman_task_free(gearman_task_st *task);

/**
 * Get callback function argument for a task.
 */
void *gearman_task_fn_arg(gearman_task_st *task);

/**
 * Get function name associated with a task.
 */
const char *gearman_task_function(gearman_task_st *task);

/**
 * Get unique identifier for a task.
 */
const char *gearman_task_uuid(gearman_task_st *task);

/**
 * Get job handle for a task.
 */
const char *gearman_task_job_handle(gearman_task_st *task);

/**
 * Get status on whether a task is known or not.
 */
bool gearman_task_is_known(gearman_task_st *task);

/**
 * Get status on whether a task is running or not.
 */
bool gearman_task_is_running(gearman_task_st *task);

/**
 * Get the numerator of percentage complete for a task.
 */
uint32_t gearman_task_numerator(gearman_task_st *task);

/**
 * Get the denominator of percentage complete for a task.
 */
uint32_t gearman_task_denominator(gearman_task_st *task);

/**
 * Get data being returned for a task.
 */
const void *gearman_task_data(gearman_task_st *task);

/**
 * Get data size being returned for a task.
 */
size_t gearman_task_data_size(gearman_task_st *task);

/**
 * Read work or result data into a buffer for a task.
 */
size_t gearman_task_recv_data(gearman_task_st *task, void *data,
                              size_t data_size, gearman_return_t *ret_ptr);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_TASK_H__ */
