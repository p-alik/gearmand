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

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup gearman_con Connection Declarations
 * @ingroup gearman_universal
 *
 * This is a low level interface for gearman connections. This is used
 * internally by both client and worker interfaces, so you probably want to
 * look there first. This is usually used to write lower level clients, workers,
 * proxies, or your own server.
 *
 * @{
 */

/**
 * @ingroup gearman_connection
 */

/** Initialize a connection structure. Always check the return value even if
 * passing in a pre-allocated structure. Some other initialization may have
 * failed.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] connection Caller allocated structure, or NULL to allocate one.
 * @return On success, a pointer to the (possibly allocated) structure. On
 *  failure this will be NULL.
 */
GEARMAN_INTERNAL_API
  void gearmand_connection_init(gearmand_connection_list_st *gearman,
                                gearmand_io_st *connection,
                                struct gearmand_con_st *dcon,
                                gearmand_connection_options_t *options);

/**
 * Free a connection structure.
 *
 * @param[in] connection Structure previously initialized with gearmand_connection_init(),
 *  gearmand_connection_init_args(), or gearman_connection_clone().
 */
GEARMAN_INTERNAL_API
void gearmand_io_free(gearmand_io_st *connection);


GEARMAN_INTERNAL_API
gearmand_error_t gearman_io_set_option(gearmand_io_st *connection,
                                               gearmand_connection_options_t options,
                                               bool value);


/**
 * Set connection to an already open file descriptor.
 */
GEARMAN_INTERNAL_API
gearmand_error_t gearman_io_set_fd(gearmand_io_st *connection, int fd);

/**
 * Get application context pointer.
 */
GEARMAN_INTERNAL_API
gearmand_con_st *gearman_io_context(const gearmand_io_st *connection);

/**
 * Used by thread to send packets.
 */
GEARMAN_INTERNAL_API
gearmand_error_t gearman_io_send(gearman_server_con_st *connection,
                                 const gearmand_packet_st *packet, bool flush);

/**
 * Used by thread to recv packets.
 */
GEARMAN_INTERNAL_API
gearmand_error_t gearman_io_recv(gearman_server_con_st *con, bool recv_data);

/**
 * Set events to be watched for a connection.
 */
GEARMAN_INTERNAL_API
gearmand_error_t gearmand_io_set_events(gearman_server_con_st *connection, short events);

/**
 * Set events that are ready for a connection. This is used with the external
 * event callbacks.
 */
GEARMAN_INTERNAL_API
gearmand_error_t gearmand_io_set_revents(gearman_server_con_st *connection, short revents);

GEARMAN_INTERNAL_API
void gearmand_sockfd_close(int sockfd);

GEARMAN_INTERNAL_API
void gearmand_pipe_close(int sockfd);

/** @} */

#ifdef __cplusplus
}
#endif
