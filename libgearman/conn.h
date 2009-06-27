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
GEARMAN_API
gearman_con_st *gearman_con_add(gearman_st *gearman, gearman_con_st *con,
                                const char *host, in_port_t port);

/**
 * Initialize a connection structure.
 */
GEARMAN_API
gearman_con_st *gearman_con_create(gearman_st *gearman, gearman_con_st *con);

/**
 * Clone a connection structure.
 */
GEARMAN_API
gearman_con_st *gearman_con_clone(gearman_st *gearman, gearman_con_st *con,
                                  gearman_con_st *from);

/**
 * Free a connection structure.
 */
GEARMAN_API
void gearman_con_free(gearman_con_st *con);

/**
 * Set options for a connection.
 */
GEARMAN_API
void gearman_con_set_host(gearman_con_st *con, const char *host);
GEARMAN_API
void gearman_con_set_port(gearman_con_st *con, in_port_t port);
GEARMAN_API
void gearman_con_set_options(gearman_con_st *con, gearman_con_options_t options,
                             uint32_t data);

/**
 * Set connection to an already open file descriptor.
 */
GEARMAN_API
gearman_return_t gearman_con_set_fd(gearman_con_st *con, int fd);

/**
 * Get application data pointer.
 */
GEARMAN_API
void *gearman_con_data(gearman_con_st *con);

/**
 * Set application data pointer.
 */
GEARMAN_API
void gearman_con_set_data(gearman_con_st *con, void *data);

/**
 * Connect to server.
 */
GEARMAN_API
gearman_return_t gearman_con_connect(gearman_con_st *con);

/**
 * Close a connection.
 */
GEARMAN_API
void gearman_con_close(gearman_con_st *con);

/**
 * Clear address info, freeing structs if needed.
 */
GEARMAN_API
void gearman_con_reset_addrinfo(gearman_con_st *con);

/**
 * Send packet to a connection.
 */
GEARMAN_API
gearman_return_t gearman_con_send(gearman_con_st *con,
                                  gearman_packet_st *packet, bool flush);

/**
 * Send packet data to a connection.
 */
GEARMAN_API
size_t gearman_con_send_data(gearman_con_st *con, const void *data,
                             size_t data_size, gearman_return_t *ret_ptr);

/**
 * Flush the send buffer.
 */
GEARMAN_API
gearman_return_t gearman_con_flush(gearman_con_st *con);

/**
 * Flush the send buffer for all connections.
 */
GEARMAN_API
gearman_return_t gearman_con_flush_all(gearman_st *gearman);

/**
 * Send packet to all connections.
 */
GEARMAN_API
gearman_return_t gearman_con_send_all(gearman_st *gearman,
                                      gearman_packet_st *packet);

/**
 * Receive packet from a connection.
 */
GEARMAN_API
gearman_packet_st *gearman_con_recv(gearman_con_st *con,
                                    gearman_packet_st *packet,
                                    gearman_return_t *ret_ptr, bool recv_data);

/**
 * Receive packet data from a connection.
 */
GEARMAN_API
size_t gearman_con_recv_data(gearman_con_st *con, void *data, size_t data_size,
                             gearman_return_t *ret_ptr);

/**
 * Read data from a connection.
 */
GEARMAN_API
size_t gearman_con_read(gearman_con_st *con, void *data, size_t data_size,
                        gearman_return_t *ret_ptr);

/**
 * Wait for I/O on connections.
 */
GEARMAN_API
gearman_return_t gearman_con_wait(gearman_st *gearman, int timeout);

/**
 * Set events to be watched for a connection.
 */
GEARMAN_API
gearman_return_t gearman_con_set_events(gearman_con_st *con, short events);

/**
 * Set events that are ready for a connection. This is used with the external
 * event callbacks.
 */
GEARMAN_API
gearman_return_t gearman_con_set_revents(gearman_con_st *con, short revents);

/**
 * Get next connection that is ready for I/O.
 */
GEARMAN_API
gearman_con_st *gearman_con_ready(gearman_st *gearman);

/**
 * Test echo with all connections.
 */
GEARMAN_API
gearman_return_t gearman_con_echo(gearman_st *gearman, const void *workload,
                                  size_t workload_size);

/**
 * Get protocol data pointer.
 */
GEARMAN_API
void *gearman_con_protocol_data(gearman_con_st *con);

/**
 * Set protocol data pointer.
 */
GEARMAN_API
void gearman_con_set_protocol_data(gearman_con_st *con, void *data);

/**
 * Set function to call when protocol_data should be freed.
 */
GEARMAN_API
void gearman_con_set_protocol_data_free_fn(gearman_con_st *con,
                                    gearman_con_protocol_data_free_fn *free_fn);

/**
 * Set custom recv function
 */
GEARMAN_API
void gearman_con_set_recv_fn(gearman_con_st *con, gearman_con_recv_fn recv_fn);

/**
 * Set custom recv_data function
 */
GEARMAN_API
void gearman_con_set_recv_data_fn(gearman_con_st *con,
                                  gearman_con_recv_data_fn recv_data_fn);

/**
 * Set custom send function
 */
GEARMAN_API
void gearman_con_set_send_fn(gearman_con_st *con, gearman_con_send_fn send_fn);

/**
 * Set custom send_data function
 */
GEARMAN_API
void gearman_con_set_send_data_fn(gearman_con_st *con,
                                  gearman_con_send_data_fn send_data_fn);

/**
 * Set custom packet_pack function
 */
GEARMAN_API
void gearman_con_set_packet_pack_fn(gearman_con_st *con,
                                    gearman_packet_pack_fn pack_fn);

/**
 * Set custom packet_unpack function
 */
GEARMAN_API
void gearman_con_set_packet_unpack_fn(gearman_con_st *con,
                                     gearman_packet_unpack_fn acpket_unpack_fn);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CON_H__ */
