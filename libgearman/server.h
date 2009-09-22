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

#include <libgearman/gearman.h>
#include <libgearman/server_con.h>
#include <libgearman/server_packet.h>
#include <libgearman/server_function.h>
#include <libgearman/server_client.h>
#include <libgearman/server_worker.h>
#include <libgearman/server_job.h>
#include <libgearman/server_thread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_server Server Interface
 * This is the interface gearman servers should use.
 * @{
 */

/**
 * Initialize a server structure. This cannot fail if the caller supplies a
 * server structure.
 * @param server Caller allocated server structure, or NULL to allocate one.
 * @return Pointer to an allocated server structure if server parameter was
 *         NULL, or the server parameter pointer if it was not NULL.
 */
GEARMAN_API
gearman_server_st *gearman_server_create(gearman_server_st *server);

/**
 * Free resources used by a server structure.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 */
GEARMAN_API
void gearman_server_free(gearman_server_st *server);

/**
 * Set logging callback for server instance.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @param function Function to call when there is a logging message.
 * @param context Argument to pass into the log callback function.
 * @param verbose Verbosity level.
 */
GEARMAN_API
void gearman_server_set_log_fn(gearman_server_st *server,
                               gearman_log_fn *function,
                               const void *context, gearman_verbose_t verbose);

/**
 * Process commands for a connection.
 * @param server_con Server connection that has a packet to process.
 * @param packet The packet that needs processing.
 * @return Standard gearman return value.
 */
GEARMAN_API
gearman_return_t gearman_server_run_command(gearman_server_con_st *server_con,
                                            gearman_packet_st *packet);

/**
 * Tell server that it should enter a graceful shutdown state.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @return Standard gearman return value. This will return GEARMAN_SHUTDOWN if
 *         the server is ready to shutdown now.
 */
GEARMAN_API
gearman_return_t gearman_server_shutdown_graceful(gearman_server_st *server);

/**
 * Replay the persistent queue to load all unfinshed jobs into the server. This
 * should only be run at startup.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @return Standard gearman return value. This will return GEARMAN_SHUTDOWN if
 *         the server is ready to shutdown now.
 */
GEARMAN_API
gearman_return_t gearman_server_queue_replay(gearman_server_st *server);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_H__ */
