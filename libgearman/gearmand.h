/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearmand declarations
 */

#ifndef __GEARMAND_H__
#define __GEARMAND_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearmand Gearmand
 * This is a server implementation using the gearman_server interface.
 * @{
 */

/**
 * Create a server instance.
 * @param port Port for the server to listen on.
 * @return Pointer to an allocated gearmand structure.
 */
gearmand_st *gearmand_create(in_port_t port, char *host);

/**
 * Free resources used by a server instace.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 */
void gearmand_free(gearmand_st *gearmand);

/**
 * Set socket backlog for listening connection.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param backlog Number of backlog connections to set during listen.
 */
void gearmand_set_backlog(gearmand_st *gearmand, int backlog);

/**
 * Set number of I/O threads for server to use.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param threads Number of threads.
 */
void gearmand_set_threads(gearmand_st *gearmand, uint32_t threads);

/**
 * Set verbosity level for server instance.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param verbose Verbosity level.
 */
void gearmand_set_verbose(gearmand_st *gearmand, uint8_t verbose);

/**
 * Set logging callback for server instance.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param log_fn Function to call when there is a logging message.
 * @param log_fn_arg Argument to pass into the log callback function.
 */
void gearmand_set_log(gearmand_st *gearmand, gearmand_log_fn log_fn,
                      void *log_fn_arg);

/**
 * Run the server instance.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @return Standard gearman return value.
 */
gearman_return_t gearmand_run(gearmand_st *gearmand);

/**
 * Interrupt a running gearmand server from another thread. You should only
 * call this when another thread is currently running gearmand_run() and you
 * want to wakeup the server with the given event.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param wakeup Wakeup event to send to running gearmand.
 */
void gearmand_wakeup(gearmand_st *gearmand, gearmand_wakeup_t wakeup);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAND_H__ */
