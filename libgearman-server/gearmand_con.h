/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Connection Declarations
 */

#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <libgearman-server/struct/gearmand_con.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearmand_con Connection Declarations
 * @ingroup gearmand
 *
 * Connection handling for gearmand.
 *
 * @{
 */

/**
 * Create a new gearmand connection.
 * @param gearmand Server instance structure previously initialized with
 *        gearmand_create.
 * @param fd File descriptor of new connection.
 * @param host Host of peer connection.
 * @param port Port of peer connection.
 * @param add_fn Optional callback to use when adding the connection to an
          I/O thread.
 * @return Pointer to an allocated gearmand structure.
 */
GEARMAN_API
gearmand_error_t gearmand_con_create(gearmand_st *gearmand, int fd,
                                     const char *host, const char *port,
                                     gearmand_connection_add_fn *add_fn);

/**
 * Free resources used by a connection.
 * @param dcon Connection previously initialized with gearmand_con_create.
 */
GEARMAN_API
void gearmand_con_free(gearmand_con_st *dcon);

/**
 * Check connection queue for a thread.
 */
GEARMAN_API
void gearmand_con_check_queue(gearmand_thread_st *thread);

/**
 * Callback function used for setting events in libevent.
 */
GEARMAN_API
gearmand_error_t gearmand_connection_watch(gearmand_io_st *con, short events,
                                           void *context);

/** @} */

gearmand_con_st* build_gearmand_con_st(void);

void destroy_gearmand_con_st(gearmand_con_st*);
void _con_ready(int fd, short events, void *arg);

#ifdef __cplusplus
}
#endif
