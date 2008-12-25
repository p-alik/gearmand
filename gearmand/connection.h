/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
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
