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

#include <libgearman-server/struct/server.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

inline static void gearmand_set_round_robin(gearman_server_st *server, bool round_robin)
{
  server->flags.round_robin= round_robin;
}

/**
 * Process commands for a connection.
 * @param server_con Server connection that has a packet to process.
 * @param packet The packet that needs processing.
 * @return Standard gearman return value.
 */
GEARMAN_API
gearmand_error_t gearman_server_run_command(gearman_server_con_st *server_con,
                                            gearmand_packet_st *packet);

/**
 * Tell server that it should enter a graceful shutdown state.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @return Standard gearman return value. This will return GEARMAN_SHUTDOWN if
 *         the server is ready to shutdown now.
 */
GEARMAN_API
gearmand_error_t gearman_server_shutdown_graceful(gearman_server_st *server);

/**
 * Replay the persistent queue to load all unfinshed jobs into the server. This
 * should only be run at startup.
 * @param server Server structure previously initialized with
 *        gearman_server_create.
 * @return Standard gearman return value. This will return GEARMAN_SHUTDOWN if
 *         the server is ready to shutdown now.
 */
GEARMAN_API
gearmand_error_t gearman_server_queue_replay(gearman_server_st *server);

/**
 * Get persistent queue context.
 */
GEARMAN_API
void *gearman_server_queue_context(const gearman_server_st *server);

/** @} */

#ifdef __cplusplus
}
#endif
