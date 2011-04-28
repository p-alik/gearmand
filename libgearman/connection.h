/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2008 Brian Aker, Eric Day
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 * @brief Connection Declarations
 */

#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

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

enum gearman_con_recv_t {
  GEARMAN_CON_RECV_UNIVERSAL_NONE,
  GEARMAN_CON_RECV_UNIVERSAL_READ,
  GEARMAN_CON_RECV_STATE_READ_DATA
};

enum gearman_con_send_t {
  GEARMAN_CON_SEND_STATE_NONE,
  GEARMAN_CON_SEND_UNIVERSAL_PRE_FLUSH,
  GEARMAN_CON_SEND_UNIVERSAL_FORCE_FLUSH,
  GEARMAN_CON_SEND_UNIVERSAL_FLUSH,
  GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA
};

enum gearman_con_universal_t {
  GEARMAN_CON_UNIVERSAL_ADDRINFO,
  GEARMAN_CON_UNIVERSAL_CONNECT,
  GEARMAN_CON_UNIVERSAL_CONNECTING,
  GEARMAN_CON_UNIVERSAL_CONNECTED
};

/**
 * @ingroup gearman_connection
 */

#ifdef GEARMAN_CORE

/**
 * Initialize a connection structure. Always check the return value even if
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
gearman_connection_st *gearman_connection_create(gearman_universal_st *gearman,
                                                 gearman_connection_st *connection,
                                                 gearman_connection_options_t *options);

/**
 * Create a connection structure with the given host and port.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] connection Caller allocated structure, or NULL to allocate one.
 * @param[in] host Host or IP address to connect to.
 * @param[in] port Port to connect to.
 * @return On success, a pointer to the (possibly allocated) structure. On
 *  failure this will be NULL.
 */
GEARMAN_INTERNAL_API
gearman_connection_st *gearman_connection_create_args(gearman_universal_st *gearman,
                                                      gearman_connection_st *connection,
                                                      const char *host, in_port_t port);

/**
 * Clone a connection structure.
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] connection Caller allocated structure, or NULL to allocate one.
 * @param[in] from Structure to use as a source to clone from.
 * @return On success, a pointer to the (possibly allocated) structure. On
 *  failure this will be NULL.
 */
GEARMAN_INTERNAL_API
gearman_connection_st *gearman_connection_clone(gearman_universal_st *gearman, gearman_connection_st *src,
                                                const gearman_connection_st *from);

/**
 * Free a connection structure.
 *
 * @param[in] connection Structure previously initialized with gearman_connection_create(),
 *  gearman_connection_create_args(), or gearman_connection_clone().
 */
GEARMAN_INTERNAL_API
void gearman_connection_free(gearman_connection_st *connection);


/**
 * Set host for a connection.
 */
GEARMAN_INTERNAL_API
void gearman_connection_set_host(gearman_connection_st *connection,
                                 const char *host,
                                 in_port_t port);

/**
 * Close a connection.
 */
GEARMAN_INTERNAL_API
void gearman_connection_close(gearman_connection_st *connection);

/**
 * Send packet to a connection.
 */
GEARMAN_INTERNAL_API
gearman_return_t gearman_connection_send(gearman_connection_st *connection,
                                         const gearman_packet_st *packet, bool flush);

/**
 * Send packet data to a connection.
 */
GEARMAN_INTERNAL_API
size_t gearman_connection_send_data(gearman_connection_st *connection, const void *data,
                                    size_t data_size, gearman_return_t *ret_ptr);

/**
 * Flush the send buffer.
 */
GEARMAN_INTERNAL_API
gearman_return_t gearman_connection_flush(gearman_connection_st *connection);

/**
 * Receive packet from a connection.
 */
GEARMAN_INTERNAL_API
gearman_packet_st *gearman_connection_recv(gearman_connection_st *connection,
                                           gearman_packet_st *packet,
                                           gearman_return_t *ret_ptr, bool recv_data);

/**
 * Receive packet data from a connection.
 */
GEARMAN_INTERNAL_API
size_t gearman_connection_recv_data(gearman_connection_st *connection, void *data, size_t data_size,
                                    gearman_return_t *ret_ptr);

/**
 * Read data from a connection.
 */
GEARMAN_INTERNAL_API
size_t gearman_connection_read(gearman_connection_st *connection, void *data, size_t data_size,
                               gearman_return_t *ret_ptr);

/**
 * Set events to be watched for a connection.
 */
GEARMAN_INTERNAL_API
gearman_return_t gearman_connection_set_events(gearman_connection_st *connection, short events);

/**
 * Set events that are ready for a connection. This is used with the external
 * event callbacks.
 */
GEARMAN_INTERNAL_API
gearman_return_t gearman_connection_set_revents(gearman_connection_st *connection, short revents);

/** @} */

#endif /* GEARMAN_CORE */
