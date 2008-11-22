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
 * @defgroup gearman_client Client Interface
 * This is the interface gearman clients should use. You can run tasks one at a
 * time or concurrently.
 * @{
 */

/**
 * Initialize a client structure. This cannot fail if the caller supplies a
 * client structure.
 * @param gearman Gearman instance if one exists, otherwise pass in NULL to
 *        create one.
 * @param client Caller allocated client structure, or NULL to allocate one.
 * @return Pointer to an allocated client structure if client parameter was
 *         NULL, or the client parameter pointer if it was not NULL.
 */
gearman_client_st *gearman_client_create(gearman_st *gearman,
                                         gearman_client_st *client);

/**
 * Clone a client structure.
 * @param gearman Gearman instance if one exists, otherwise pass in NULL to
 *        create a clone from the existing one used by from.
 * @param client Caller allocated client structure, or NULL to allocate one.
 * @param from Client structure to use as a source to clone from.
 * @return Pointer to an allocated client structure if client parameter was
 *         NULL, or the client parameter pointer if it was not NULL.
 */
gearman_client_st *gearman_client_clone(gearman_st *gearman,
                                        gearman_client_st *client,
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
 * @param options Available options for gearman structs.
 * @param data For options that require parameters, the value of that parameter.
 *        For all other option flags, this should be 0 to clear the option or 1
 *        to set.
 */
void gearman_client_set_options(gearman_client_st *client,
                                gearman_options options, uint32_t data);

/**
 * Add a job server to a client. This goes into a list of servers than can be
 * used to run tasks. No socket I/O happens here, it is just added to a list.
 * @param client Client structure previously initialized with
 *               gearman_client_create or gearman_client_clone.
 * @param host Hostname or IP address (IPv4 or IPv6) of the server to add.
 * @param port Port of the server to add.
 * @return Standard gearman return value.
 */
gearman_return gearman_client_server_add(gearman_client_st *client, char *host,
                                         in_port_t port);

/**
 * @defgroup gearman_client_single Single Task Interface
 * @ingroup gearman_client
 * Use the following set of functions to run one task at a time.
 * @{
 */

/** Run a task, returns allocated result. */
void *gearman_client_do(gearman_client_st *client, const char *function_name,
                        const void *workload, size_t workload_size,
                        size_t *result_size, gearman_return *ret_ptr);

/** Run a high priority task, returns allocated result. */
void *gearman_client_do_high(gearman_client_st *client,
                             const char *function_name, const void *workload,
                             size_t workload_size, size_t *result_size,
                             gearman_return *ret_ptr);

/** Run a task in the background. The job_handle_buffer must be at least
   GEARMAN_JOB_HANDLE_SIZE bytes. */
gearman_return gearman_client_do_bg(gearman_client_st *client,
                                    const char *function_name,
                                    const void *workload, size_t workload_size,
                                    char *job_handle_buffer);

/** Get the status for a backgound task. */
gearman_return gearman_client_task_status(gearman_client_st *client,
                                          const char *job_handle,
                                          bool *is_known, bool *is_running,
                                          uint32_t *numerator,
                                          uint32_t *denominator);

/** Send data to all job servers to see if they echo it back. */
gearman_return gearman_client_echo(gearman_client_st *client,
                                   const void *workload, size_t workload_size);

/** @} */

/**
 * @defgroup gearman_client_concurrent Concurrent Task Interface
 * @ingroup gearman_client
 * Use the following set of functions to multiple run tasks concurrently.
 * @{
 */

/** Add a task to be run in parallel. */
gearman_task_st *gearman_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         const void *fn_arg,
                                         const char *function_name,
                                         const void *workload,
                                         size_t workload_size,
                                         gearman_return *ret_ptr);

/** Add a high priority task to be run in parallel. */
gearman_task_st *gearman_client_add_task_high(gearman_client_st *client,
                                              gearman_task_st *task,
                                              const void *fn_arg,
                                              const char *function_name,
                                              const void *workload,
                                              size_t workload_size,
                                              gearman_return *ret_ptr);

/** Add a background task to be run in parallel. */
gearman_task_st *gearman_client_add_task_bg(gearman_client_st *client,
                                            gearman_task_st *task,
                                            const void *fn_arg,
                                            const char *function_name,
                                            const void *workload,
                                            size_t workload_size,
                                            gearman_return *ret_ptr);

/** Add task to get the status for a backgound task in parallel. */
gearman_task_st *gearman_client_add_task_status(gearman_client_st *client,
                                                gearman_task_st *task,
                                                const void *fn_arg,
                                                const char *job_handle,
                                                gearman_return *ret_ptr);

/** Run tasks that have been added in parallel. */
gearman_return gearman_client_run_tasks(gearman_client_st *client,
                                        gearman_workload_fn workload_fn,
                                        gearman_created_fn created_fn,
                                        gearman_data_fn data_fn,
                                        gearman_status_fn status_fn,
                                        gearman_complete_fn complete_fn,
                                        gearman_fail_fn fail_fn);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CLIENT_H__ */
