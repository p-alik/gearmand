/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Connection declarations
 */

#ifndef __GEARMAN_CON_H__
#define __GEARMAN_CON_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_con Connection Handling
 * This is a low level interface for gearman connections. This is used
 * internally by both client and worker interfaces, so you probably want to
 * look there first. This is usually used to write lower level clients, workers,
 * proxies, or your own server.
 * @{
 */

/**
 * Common usage, create a connection structure with the given host:port.
 */
gearman_con_st *gearman_con_add(gearman_st *gearman, gearman_con_st *con,
                                const char *host, in_port_t port);

/**
 * Initialize a connection structure.
 */
gearman_con_st *gearman_con_create(gearman_st *gearman, gearman_con_st *con);

/**
 * Clone a connection structure.
 */
gearman_con_st *gearman_con_clone(gearman_st *gearman, gearman_con_st *con,
                                  gearman_con_st *from);

/**
 * Free a connection structure.
 */
gearman_return_t gearman_con_free(gearman_con_st *con);

/**
 * Set options for a connection.
 */
void gearman_con_set_host(gearman_con_st *con, const char *host);
void gearman_con_set_port(gearman_con_st *con, in_port_t port);
void gearman_con_set_options(gearman_con_st *con, gearman_con_options_t options,
                             uint32_t data);

/**
 * Set connection to an already open file descriptor.
 */
gearman_return_t gearman_con_set_fd(gearman_con_st *con, int fd);

/**
 * Get application data pointer.
 */
void *gearman_con_data(gearman_con_st *con);

/**
 * Set application data pointer.
 */
void gearman_con_set_data(gearman_con_st *con, void *data);

/**
 * Connect to server.
 */
gearman_return_t gearman_con_connect(gearman_con_st *con);

/**
 * Close a connection.
 */
gearman_return_t gearman_con_close(gearman_con_st *con);

/**
 * Clear address info, freeing structs if needed.
 */
void gearman_con_reset_addrinfo(gearman_con_st *con);

/**
 * Send packet to a connection.
 */
gearman_return_t gearman_con_send(gearman_con_st *con,
                                  gearman_packet_st *packet, bool flush);

/**
 * Send packet data to a connection.
 */
size_t gearman_con_send_data(gearman_con_st *con, const void *data,
                             size_t data_size, gearman_return_t *ret_ptr);

/**
 * Flush the send buffer.
 */
gearman_return_t gearman_con_flush(gearman_con_st *con);

/**
 * Flush the send buffer for all connections.
 */
gearman_return_t gearman_con_flush_all(gearman_st *gearman);

/**
 * Send packet to all connections.
 */
gearman_return_t gearman_con_send_all(gearman_st *gearman,
                                      gearman_packet_st *packet);

/**
 * Receive packet from a connection.
 */
gearman_packet_st *gearman_con_recv(gearman_con_st *con,
                                    gearman_packet_st *packet,
                                    gearman_return_t *ret_ptr, bool recv_data);

/**
 * Receive packet data from a connection.
 */
size_t gearman_con_recv_data(gearman_con_st *con, void *data, size_t data_size,
                             gearman_return_t *ret_ptr);

/**
 * Wait for I/O on connections.
 */
gearman_return_t gearman_con_wait(gearman_st *gearman, int timeout);

/**
 * Set events to be watched for a connection.
 */
gearman_return_t gearman_con_set_events(gearman_con_st *con, short events);

/**
 * Set events that are ready for a connection. This is used with the external
 * event callbacks.
 */
void gearman_con_set_revents(gearman_con_st *con, short revents);

/**
 * Get next connection that is ready for I/O.
 */
gearman_con_st *gearman_con_ready(gearman_st *gearman);

/**
 * Test echo with all connections.
 */
gearman_return_t gearman_con_echo(gearman_st *gearman, const void *workload,
                                  size_t workload_size);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CON_H__ */
