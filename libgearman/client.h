/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file
 * @brief Client declarations
 */

#ifndef __GEARMAN_CLIENT_H__
#define __GEARMAN_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_client Client Interface
 * This is the interface gearman clients should use. You can run tasks one at a
 * time or concurrently.
 *
 * See Main Page for full details.
 * @{
 */

/**
 * Initialize a client structure. This cannot fail if the caller supplies a
 * client structure.
 * @param client Caller allocated client structure, or NULL to allocate one.
 * @return Pointer to an allocated client structure if client parameter was
 *         NULL, or the client parameter pointer if it was not NULL.
 */
gearman_client_st *gearman_client_create(gearman_client_st *client);

/**
 * Clone a client structure.
 * @param client Caller allocated client structure, or NULL to allocate one.
 * @param from Client structure to use as a source to clone from.
 * @return Pointer to an allocated client structure if client parameter was
 *         NULL, or the client parameter pointer if it was not NULL.
 */
gearman_client_st *gearman_client_clone(gearman_client_st *client,
                                        gearman_client_st *from);

/**
 * Free resources used by a client structure.
 * @param client Client structure previously initialized with
 *        gearman_client_create or gearman_client_clone.
 */
void gearman_client_free(gearman_client_st *client);

/**
 * Return an error string for the last error encountered.
 * @param client Client structure previously initialized with
 *        gearman_client_create or gearman_client_clone.
 * @return Pointer to static buffer in library that holds an error string.
 */
const char *gearman_client_error(gearman_client_st *client);

/**
 * Value of errno in the case of a GEARMAN_ERRNO return value.
 * @param client Client structure previously initialized with
 *        gearman_client_create or gearman_client_clone.
 * @return An errno value as defined in your system errno.h file.
 */
int gearman_client_errno(gearman_client_st *client);

/**
 * Set options for a client structure.
 * @param client Client structure previously initialized with
 *        gearman_client_create or gearman_client_clone.
 * @param options Available options for gearman_client structs.
 * @param data For options that require parameters, the value of that parameter.
 *        For all other option flags, this should be 0 to clear the option or 1
 *        to set.
 */
void gearman_client_set_options(gearman_client_st *client,
                                gearman_client_options_t options,
                                uint32_t data);

/**
 * Add a job server to a client. This goes into a list of servers than can be
 * used to run tasks. No socket I/O happens here, it is just added to a list.
 * @param client Client structure previously initialized with
 *        gearman_client_create or gearman_client_clone.
 * @param host Hostname or IP address (IPv4 or IPv6) of the server to add.
 * @param port Port of the server to add.
 * @return Standard gearman return value.
 */
gearman_return_t gearman_client_add_server(gearman_client_st *client,
                                           const char *host, in_port_t port);

/**
 * @addtogroup gearman_client_single Single Task Interface
 * @ingroup gearman_client
 * Use the following set of functions to run one task at a time.
 *
 * See Main Page for full details.
 * @{
 */

/**
 * Run a single task and return an allocated result.
 * @param client Client structure previously initialized with
 *        gearman_client_create or gearman_client_clone.
 * @param function_name The name of the function to run.
 * @param workload The workload to pass to the function when it is run.
 * @param workload_size Size of the workload.
 * @param result_size The size of the data being returned.
 * @param ret_ptr Standard gearman return value. In the case of
 *        GEARMAN_WORK_DATA or GEARMAN_WORK_STATUS, the
 *        caller should take any actions and then call this
 *        function again. This may happen multiple times until a
 *        GEARMAN_WORK_ERROR, GEARMAN_WORK_FAIL, or GEARMAN_SUCCESS
 *        (work complete) is returned. For GEARMAN_WORK_DATA,
 *        the result_size will be set to the intermediate data
 *        chunk being returned and an allocated data buffer will
 *        be returned. For GEARMAN_WORK_STATUS, the caller can use
 *        gearman_client_do_status() to get the current tasks status.
 * @return The result allocated by the library, this needs to be freed when the
 *         caller is done using it.
 */
void *gearman_client_do(gearman_client_st *client, const char *function_name,
                        const void *workload, size_t workload_size,
                        size_t *result_size, gearman_return_t *ret_ptr);

/**
 * Run a high priority task and return an allocated result. See
 * gearman_client_do() for parameter and return information.
 */
void *gearman_client_do_high(gearman_client_st *client,
                             const char *function_name, const void *workload,
                             size_t workload_size, size_t *result_size,
                             gearman_return_t *ret_ptr);

/**
 * Get the job handle for the running task. This should be used between
 * repeated gearman_client_do() and gearman_client_do_high() calls to get
 * information.
 * @param client Client structure previously initialized with
 *        gearman_client_create or gearman_client_clone.
 * @return Pointer to static buffer in library that holds the job handle.
 */
const char *gearman_client_do_job_handle(gearman_client_st *client);

/**
 * Get the status for the running task. This should be used between
 * repeated gearman_client_do() and gearman_client_do_high() calls to get
 * information.
 * @param client Client structure previously initialized with
 *        gearman_client_create or gearman_client_clone.
 * @param numerator Optional parameter to store the percent complete
          numerator in.
 * @param denominator Optional parameter to store the percent complete
          denominator in.
 */
void gearman_client_do_status(gearman_client_st *client, uint32_t *numerator,
                              uint32_t *denominator);

/**
 * Run a task in the background.
 * @param client Client structure previously initialized with
 *        gearman_client_create or gearman_client_clone.
 * @param function_name The name of the function to run.
 * @param workload The workload to pass to the function when it is run.
 * @param workload_size Size of the workload.
 * @param job_handle A buffer to store the job handle in.
 * @return Standard gearman return value.
 */
gearman_return_t gearman_client_do_background(gearman_client_st *client,
                                              const char *function_name,
                                              const void *workload,
                                              size_t workload_size,
                                              gearman_job_handle_t job_handle);

/**
 * Get the status for a backgound task.
 * @param client Client structure previously initialized with
 *        gearman_client_create or gearman_client_clone.
 * @param job_handle The job handle you want status for.
 * @param is_known Optional parameter to store the known status in.
 * @param is_running Optional parameter to store the running status in.
 * @param numerator Optional parameter to store the percent complete
          numerator in.
 * @param denominator Optional parameter to store the percent complete
          denominator in.
 * @return Standard gearman return value.
 */
gearman_return_t gearman_client_task_status(gearman_client_st *client,
                                            const char *job_handle,
                                            bool *is_known, bool *is_running,
                                            uint32_t *numerator,
                                            uint32_t *denominator);

/**
 * Send data to all job servers to see if they echo it back. This is a test
 * function to see if job servers are responding properly.
 * @param client Client structure previously initialized with
 *        gearman_client_create or gearman_client_clone.
 * @param workload The workload to ask the server to echo back.
 * @param workload_size Size of the workload.
 * @return Standard gearman return value.
 */
gearman_return_t gearman_client_echo(gearman_client_st *client,
                                     const void *workload,
                                     size_t workload_size);

/** @} */

/**
 * @addtogroup gearman_client_concurrent Concurrent Task Interface
 * @ingroup gearman_client
 * Use the following set of functions to multiple run tasks concurrently.
 *
 * See Main Page for full details.
 * @{
 */

/**
 * Add a task to be run in parallel.
 */
gearman_task_st *gearman_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         const void *fn_arg,
                                         const char *function_name,
                                         const void *workload,
                                         size_t workload_size,
                                         gearman_return_t *ret_ptr);

/**
 * Add a high priority task to be run in parallel.
 */
gearman_task_st *gearman_client_add_task_high(gearman_client_st *client,
                                              gearman_task_st *task,
                                              const void *fn_arg,
                                              const char *function_name,
                                              const void *workload,
                                              size_t workload_size,
                                              gearman_return_t *ret_ptr);

/**
 * Add a background task to be run in parallel.
 */
gearman_task_st *gearman_client_add_task_background(gearman_client_st *client,
                                                    gearman_task_st *task,
                                                    const void *fn_arg,
                                                    const char *function_name,
                                                    const void *workload,
                                                    size_t workload_size,
                                                    gearman_return_t *ret_ptr);

/**
 * Add task to get the status for a backgound task in parallel.
 */
gearman_task_st *gearman_client_add_task_status(gearman_client_st *client,
                                                gearman_task_st *task,
                                                const void *fn_arg,
                                                const char *job_handle,
                                                gearman_return_t *ret_ptr);

/**
 * Run tasks that have been added in parallel.
 */
gearman_return_t gearman_client_run_tasks(gearman_client_st *client,
                                          gearman_workload_fn *workload_fn,
                                          gearman_created_fn *created_fn,
                                          gearman_data_fn *data_fn,
                                          gearman_status_fn *status_fn,
                                          gearman_complete_fn *complete_fn,
                                          gearman_fail_fn *fail_fn);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CLIENT_H__ */
