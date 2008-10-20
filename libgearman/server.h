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

#ifndef __LIBGEARMAN_SERVER_H__
#define __LIBGEARMAN_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <inttypes.h>

#include <libgearman/constants.h>
#include <libgearman/types.h>
#include <libgearman/watchpoint.h>

struct gearman_server_list_st {
  gearman_server_st *hosts;
  uint32_t number_of_hosts;
  int32_t poll_timeout;
};

/* You should never reference this directly */
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
  bool sockaddr_inited;
  struct addrinfo *address_info;
  time_t next_retry;
  gearman_server_type type;
  gearman_server_list_st *root;
};

gearman_return gearman_server_add(gearman_server_list_st *ptr,
                                  const char *hostname,
                                  uint16_t port);
gearman_return gearman_server_copy(gearman_server_list_st *destination, 
                                   gearman_server_list_st *source);
void gearman_server_list_free(gearman_server_list_st *list);

#ifdef GEARMAN_INTERNAL 

#endif /* GEARMAN_INTERNAL */

#define gearman_server_list_count(A) (A)->number_of_hosts
#define gearman_server_name(A,B) (B).hostname
#define gearman_server_port(A,B) (B).port
#define gearman_server_list(A) (A)->hosts
#define gearman_server_response_count(A) (A)->cursor_active

#ifdef __cplusplus
}
#endif

#endif /* __LIBGEARMAN_SERVER_H__ */
