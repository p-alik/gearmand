/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Worker declarations
 */

#ifndef __GEARMAN_WORKER_H__
#define __GEARMAN_WORKER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_worker Worker Interface
 * This is the interface gearman workers should use.
 *
 * @ref main_page_worker "See Main Page for full details."
 * @{
 */

/**
 * Initialize a worker structure. This cannot fail if the caller supplies a
 * worker structure.
 * @param worker Caller allocated worker structure, or NULL to allocate one.
 * @return Pointer to an allocated worker structure if worker parameter was
 *         NULL, or the worker parameter pointer if it was not NULL.
 */
gearman_worker_st *gearman_worker_create(gearman_worker_st *worker);

/**
 * Clone a worker structure.
 * @param worker Caller allocated worker structure, or NULL to allocate one.
 * @param from Worker structure to use as a source to clone from.
 * @return Pointer to an allocated worker structure if worker parameter was
 *         NULL, or the worker parameter pointer if it was not NULL.
 */
gearman_worker_st *gearman_worker_clone(gearman_worker_st *worker,
                                        gearman_worker_st *from);

/**
 * Free resources used by a worker structure.
 * @param worker Worker structure previously initialized with
 *        gearman_worker_create or gearman_worker_clone.
 */
void gearman_worker_free(gearman_worker_st *worker);

/**
 * Return an error string for the last error encountered.
 * @param worker Worker structure previously initialized with
 *        gearman_worker_create or gearman_worker_clone.
 * @return Pointer to static buffer in library that holds an error string.
 */
const char *gearman_worker_error(gearman_worker_st *worker);

/**
 * Value of errno in the case of a GEARMAN_ERRNO return value.
 * @param worker Worker structure previously initialized with
 *        gearman_worker_create or gearman_worker_clone.
 * @return An errno value as defined in your system errno.h file.
 */
int gearman_worker_errno(gearman_worker_st *worker);

/**
 * Set options for a worker structure.
 * @param worker Worker structure previously initialized with
 *        gearman_worker_create or gearman_worker_clone.
 * @param options Available options for gearman_worker structs.
 * @param data For options that require parameters, the value of that parameter.
 *        For all other option flags, this should be 0 to clear the option or 1
 *        to set.
 */
void gearman_worker_set_options(gearman_worker_st *worker,
                                gearman_worker_options_t options,
                                uint32_t data);

/**
 * Set custom memory allocation function for workloads. Normally gearman uses
 * the standard system malloc to allocate memory used with workloads. This
 * function is used instead.
 * @param worker Worker structure previously initialized with
 *        gearman_worker_create or gearman_worker_clone.
 * @param workload_malloc Memory allocation function to replace malloc().
 * @param workload_malloc_arg Argument to pass along to workload_malloc.
 */
 
void gearman_worker_set_workload_malloc(gearman_worker_st *worker,
                                        gearman_malloc_fn *workload_malloc,
                                        const void *workload_malloc_arg);
 
/**
 * Set custom memory free function for workloads. Normally gearman uses the
 * standard system free to free memory used with workloads. This function
 * is used instead.
 * @param worker Worker structure previously initialized with
 *        gearman_worker_create or gearman_worker_clone.
 * @param workload_free Memory free function to replace free().
 * @param workload_free_arg Argument to pass along to workload_free.
 */
 
void gearman_worker_set_workload_free(gearman_worker_st *worker,
                                      gearman_free_fn *workload_free,
                                      const void *workload_free_arg);

/**
 * Add a job server to a worker. This goes into a list of servers than can be
 * used to run tasks. No socket I/O happens here, it is just added to a list.
 * @param worker Worker structure previously initialized with
 *        gearman_worker_create or gearman_worker_clone.
 * @param host Hostname or IP address (IPv4 or IPv6) of the server to add.
 * @param port Port of the server to add.
 * @return Standard gearman return value.
 */
gearman_return_t gearman_worker_add_server(gearman_worker_st *worker,
                                           const char *host, in_port_t port);

/**
 * Register function with job servers with an optional timeout. The timeout
 * specifies how many seconds the server will wait before marking a job as
 * failed. If timeout is zero, there is no timeout.
 */
gearman_return_t gearman_worker_register(gearman_worker_st *worker,
                                         const char *function_name,
                                         uint32_t timeout);

/**
 * Unregister function with job servers.
 */
gearman_return_t gearman_worker_unregister(gearman_worker_st *worker,
                                           const char *function_name);

/**
 * Unregister all functions with job servers.
 */
gearman_return_t gearman_worker_unregister_all(gearman_worker_st *worker);

/**
 * Get a job from one of the job servers.
 */
gearman_job_st *gearman_worker_grab_job(gearman_worker_st *worker,
                                        gearman_job_st *job,
                                        gearman_return_t *ret_ptr);

/**
 * Register and add callback function for worker.
 */
gearman_return_t gearman_worker_add_function(gearman_worker_st *worker,
                                             const char *function_name,
                                             uint32_t timeout,
                                             gearman_worker_fn *worker_fn,
                                             const void *fn_arg);

/**
 * Wait for a job and call the appropriate callback function when it gets one.
 */
gearman_return_t gearman_worker_work(gearman_worker_st *worker);

/**
 * Send data to all job servers to see if they echo it back. This is a test
 * function to see if job servers are responding properly.
 * @param worker Worker structure previously initialized with
 *        gearman_worker_create or gearman_worker_clone.
 * @param workload The workload to ask the server to echo back.
 * @param workload_size Size of the workload.
 * @return Standard gearman return value.
 */
gearman_return_t gearman_worker_echo(gearman_worker_st *worker,
                                     const void *workload,
                                     size_t workload_size);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_WORKER_H__ */
