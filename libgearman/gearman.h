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

#ifndef __GEARMAN_H__
#define __GEARMAN_H__

#include <libgearman/libgearman_config.h>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <libgearman/constants.h>
#include <libgearman/types.h>
#include <libgearman/byte_array.h>
#include <libgearman/watchpoint.h>
#include <libgearman/result.h>
#include <libgearman/job.h>
#include <libgearman/worker.h>
#include <libgearman/server.h>
#include <libgearman/quit.h>
#include <libgearman/behavior.h>
#include <libgearman/dispatch.h>
#include <libgearman/response.h>
#include <libgearman/str_action.h>
#include <libgearman/echo.h>

#ifdef __cplusplus
extern "C" {
#endif

/* These are Private and should not be used by applications */
#define GEARMAN_H_VERSION_STRING_LENGTH 12

struct gearman_st {
  gearman_allocated is_allocated;
  gearman_server_st *hosts;
  unsigned int number_of_hosts;
  int cached_errno;
  uint32_t flags;
  int send_size;
  int recv_size;
  int32_t poll_timeout;
  int32_t connect_timeout;
  int32_t retry_timeout;
  gearman_result_st result;
  gearman_hash hash;
  gearman_server_distribution distribution;
  void *user_data;
};

/* Public API */
gearman_st *gearman_create(gearman_st *ptr);
void gearman_free(gearman_st *ptr);
gearman_st *gearman_clone(gearman_st *clone, gearman_st *ptr);

char *gearman_strerror(gearman_st *ptr, gearman_return rc);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_H__ */
