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

#ifndef __GEARMAN_CONNECTION_H__
#define __GEARMAN_CONNECTION_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup gearman_con Connection Declarations
 * @ingroup gearman
 *
 * This is a low level interface for gearman connections. This is used
 * internally by both client and worker interfaces, so you probably want to
 * look there first. This is usually used to write lower level clients, workers,
 * proxies, or your own server.
 *
 * @{
 */

/**
 * @ingroup gearman_con
 */
struct gearman_connection_st
{
  struct {
    bool allocated:1;
    bool ready:1;
    bool packet_in_use:1;
    bool external_fd:1;
    bool ignore_lost_connection:1;
    bool close_after_flush:1;
  } options;
  gearman_connection_state_t state;
  gearman_connection_send_state_t send_state;
  gearman_connection_recv_state_t recv_state;
  in_port_t port;
  short events;
  short revents;
  int fd;
  uint32_t created_id;
  uint32_t created_id_next;
  size_t send_buffer_size;
  size_t send_data_size;
  size_t send_data_offset;
  size_t recv_buffer_size;
  size_t recv_data_size;
  size_t recv_data_offset;
  gearman_state_st *gearman;
  gearman_connection_st *next;
  gearman_connection_st *prev;
  const void *context;
  struct addrinfo *addrinfo;
  struct addrinfo *addrinfo_next;
  uint8_t *send_buffer_ptr;
  gearman_packet_st *recv_packet;
  uint8_t *recv_buffer_ptr;
  const void *protocol_context;
  gearman_connection_protocol_context_free_fn *protocol_context_free_fn;
  gearman_packet_pack_fn *packet_pack_fn;
  gearman_packet_unpack_fn *packet_unpack_fn;
  gearman_packet_st packet;
  char host[NI_MAXHOST];
  uint8_t send_buffer[GEARMAN_SEND_BUFFER_SIZE];
  uint8_t recv_buffer[GEARMAN_RECV_BUFFER_SIZE];
};

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
GEARMAN_API
gearman_connection_st *gearman_connection_create(gearman_state_st *gearman,
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
GEARMAN_API
gearman_connection_st *gearman_connection_create_args(gearman_state_st *gearman, gearman_connection_st *connection,
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
GEARMAN_API
gearman_connection_st *gearman_connection_clone(gearman_state_st *gearman, gearman_connection_st *src,
                                                const gearman_connection_st *from);

/**
 * Free a connection structure.
 *
 * @param[in] connection Structure previously initialized with gearman_connection_create(),
 *  gearman_connection_create_args(), or gearman_connection_clone().
 */
GEARMAN_API
void gearman_connection_free(gearman_connection_st *connection);


GEARMAN_API
gearman_return_t gearman_connection_set_option(gearman_connection_st *connection,
                                               gearman_connection_options_t options,
                                               bool value);


/**
 * Set host for a connection.
 */
GEARMAN_API
void gearman_connection_set_host(gearman_connection_st *connection,
                                 const char *host,
                                 in_port_t port);

/**
 * Set connection to an already open file descriptor.
 */
GEARMAN_API
gearman_return_t gearman_connection_set_fd(gearman_connection_st *connection, int fd);

/**
 * Get application context pointer.
 */
GEARMAN_API
void *gearman_connection_context(const gearman_connection_st *connection);

/**
 * Set application context pointer.
 */
GEARMAN_API
void gearman_connection_set_context(gearman_connection_st *connection, const void *context);

/**
 * Connect to server.
 */
GEARMAN_API
gearman_return_t gearman_connection_connect(gearman_connection_st *connection);

/**
 * Close a connection.
 */
GEARMAN_API
void gearman_connection_close(gearman_connection_st *connection);

/**
 * Send packet to a connection.
 */
GEARMAN_API
gearman_return_t gearman_connection_send(gearman_connection_st *connection,
                                         const gearman_packet_st *packet, bool flush);

/**
 * Send packet data to a connection.
 */
GEARMAN_API
size_t gearman_connection_send_data(gearman_connection_st *connection, const void *data,
                                    size_t data_size, gearman_return_t *ret_ptr);

/**
 * Flush the send buffer.
 */
GEARMAN_API
gearman_return_t gearman_connection_flush(gearman_connection_st *connection);

/**
 * Receive packet from a connection.
 */
GEARMAN_API
gearman_packet_st *gearman_connection_recv(gearman_connection_st *connection,
                                           gearman_packet_st *packet,
                                           gearman_return_t *ret_ptr, bool recv_data);

/**
 * Receive packet data from a connection.
 */
GEARMAN_API
size_t gearman_connection_recv_data(gearman_connection_st *connection, void *data, size_t data_size,
                                    gearman_return_t *ret_ptr);

/**
 * Read data from a connection.
 */
GEARMAN_API
size_t gearman_connection_read(gearman_connection_st *connection, void *data, size_t data_size,
                               gearman_return_t *ret_ptr);

/**
 * Set events to be watched for a connection.
 */
GEARMAN_API
gearman_return_t gearman_connection_set_events(gearman_connection_st *connection, short events);

/**
 * Set events that are ready for a connection. This is used with the external
 * event callbacks.
 */
GEARMAN_API
gearman_return_t gearman_connection_set_revents(gearman_connection_st *connection, short revents);

/**
 * Get protocol context pointer.
 */
GEARMAN_API
void *gearman_connection_protocol_context(const gearman_connection_st *connection);

/**
 * Set protocol context pointer.
 */
GEARMAN_API
void gearman_connection_set_protocol_context(gearman_connection_st *connection, const void *context);

/**
 * Set function to call when protocol_data should be freed.
 */
GEARMAN_API
void gearman_connection_set_protocol_context_free_fn(gearman_connection_st *connection,
                                                     gearman_connection_protocol_context_free_fn *function);

/**
 * Set custom packet_pack function
 */
GEARMAN_API
void gearman_connection_set_packet_pack_fn(gearman_connection_st *connection,
                                           gearman_packet_pack_fn *function);

/**
 * Set custom packet_unpack function
 */
GEARMAN_API
void gearman_connection_set_packet_unpack_fn(gearman_connection_st *connection,
                                             gearman_packet_unpack_fn *function);

/** @} */

#endif /* GEARMAN_CORE */

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CONNECTION_H__ */
