/* Gearman server and library
 * Copyright (C) 2008 Brian Aker
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

#ifndef __GEARMAN_SERVER_H__
#define __GEARMAN_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

struct gearman_server_st {
  char hostname[GEARMAN_MAX_HOST_LENGTH];
  unsigned int port;
  uint8_t io_death;
  int fd;
  int cached_errno;
  unsigned int cursor_active;
  char write_buffer[GEARMAN_MAX_BUFFER];
  size_t write_buffer_offset;
  char read_buffer[GEARMAN_MAX_BUFFER];
  size_t read_data_length;
  size_t read_buffer_length;
  char *read_ptr;
  gearman_allocated sockaddr_inited;
  struct addrinfo *address_info;
  time_t next_retry;
  gearman_server_type type;
  gearman_st *root;
};

gearman_return gearman_server_add(gearman_st *ptr, char *hostname,
                                  unsigned int port);

#ifdef GEARMAN_INTERNAL 

gearman_return gearman_server_add_internal(gearman_server_st *ptr, int fd);
gearman_return gearman_server_push(gearman_st *ptr, gearman_st *list);
void server_list_free(gearman_st *ptr, gearman_server_st *servers);
void gearman_server_free(gearman_server_st *ptr);
bool gearman_server_buffered(gearman_server_st *ptr);

#endif /* GEARMAN_INTERNAL */

#define gearman_server_count(A) (A)->number_of_hosts
#define gearman_server_name(A,B) (B).hostname
#define gearman_server_port(A,B) (B).port
#define gearman_server_list(A) (A)->hosts
#define gearman_server_response_count(A) (A)->cursor_active

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_H__ */
