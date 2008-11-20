/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __GEARMAN_CON_H__
#define __GEARMAN_CON_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Common usage, add a connection structure. */
gearman_con_st *gearman_con_add(gearman_st *gearman, gearman_con_st *con,
                                char *host, in_port_t port);

/* Initialize a connection structure. */
gearman_con_st *gearman_con_create(gearman_st *gearman, gearman_con_st *con);

/* Clone a connection structure. */
gearman_con_st *gearman_con_clone(gearman_st *gearman, gearman_con_st *con,
                                  gearman_con_st *from);

/* Free a connection structure. */
gearman_return gearman_con_free(gearman_con_st *con);

/* Set options for a connection. */
void gearman_con_set_host(gearman_con_st *con, char *host);
void gearman_con_set_port(gearman_con_st *con, in_port_t port);
void gearman_con_set_options(gearman_con_st *con, gearman_con_options options,
                             uint32_t data);

/* Set connection to an already open file descriptor. */
void gearman_con_set_fd(gearman_con_st *con, int fd);

/* Connect to server. */
gearman_return gearman_con_connect(gearman_con_st *con);

/* Close a connection. */
gearman_return gearman_con_close(gearman_con_st *con);

/* Clear address info, freeing structs if needed. */
void gearman_con_reset_addrinfo(gearman_con_st *con);

/* Send packet to a connection. */
gearman_return gearman_con_send(gearman_con_st *con, gearman_packet_st *packet,
                                bool flush);

/* Send packet data to a connection. */
size_t gearman_con_send_data(gearman_con_st *con, const void *data,
                             size_t data_size, gearman_return *ret_ptr);

/* Flush the send buffer. */
gearman_return gearman_con_flush(gearman_con_st *con);

/* Send packet to all connections. */
gearman_return gearman_con_send_all(gearman_st *gearman,
                                    gearman_packet_st *packet);

/* Receive packet from a connection. */
gearman_packet_st *gearman_con_recv(gearman_con_st *con,
                                    gearman_packet_st *packet,
                                    gearman_return *ret_ptr, bool recv_data);

/* Receive packet data from a connection. */
size_t gearman_con_recv_data(gearman_con_st *con, void *data, size_t data_size,
                             gearman_return *ret_ptr);

/* Wait for I/O on connections. */
gearman_return gearman_con_wait(gearman_st *gearman, bool set_read);

/* Get next connection that is ready for I/O. */
gearman_con_st *gearman_con_ready(gearman_st *gearman);

/* Data structures. */
struct gearman_con_st
{
  gearman_st *gearman;
  gearman_con_st *next;
  gearman_con_st *prev;
  gearman_con_state state;
  gearman_con_options options;
  void *data;
  char host[NI_MAXHOST];
  in_port_t port;
  struct addrinfo *addrinfo;
  struct addrinfo *addrinfo_next;
  int fd;
  short events;
  short revents;
  uint32_t created_id;
  uint32_t created_id_next;

  gearman_con_send_state send_state;
  uint8_t send_buffer[GEARMAN_SEND_BUFFER_SIZE];
  uint8_t *send_buffer_ptr;
  size_t send_buffer_size;
  size_t send_data_size;
  size_t send_data_offset;

  gearman_con_recv_state recv_state;
  gearman_packet_st *recv_packet;
  uint8_t recv_buffer[GEARMAN_RECV_BUFFER_SIZE];
  uint8_t *recv_buffer_ptr;
  size_t recv_buffer_size;
  size_t recv_data_size;
  size_t recv_data_offset;
};

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CON_H__ */
