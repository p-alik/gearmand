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

#ifndef __GEARMAN_CONNECTION_H__
#define __GEARMAN_CONNECTION_H__

#include <sys/types.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif
#define GEARMAN_CONNECTION_MAX_FDS 10

typedef struct gearman_connection_st gearman_connection_st;

struct gearman_connection_st {
  bool is_allocated;
  int fds[GEARMAN_CONNECTION_MAX_FDS]; 
  gearman_connection_state state;
  struct event evfifo;
  gearman_server_st server;
  gearman_result_st result;
};

gearman_connection_st *gearman_connection_create(gearman_connection_st *ptr);
void gearman_connection_free(gearman_connection_st *ptr);
gearman_connection_st *gearman_connection_clone(gearman_connection_st *clone, gearman_connection_st *ptr);
bool gearman_connection_add_fd(gearman_connection_st *ptr, int fd);
bool gearman_connection_buffered(gearman_connection_st *ptr);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_CONNECTION_H__ */
