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

#ifndef __GEARMAN_H__
#define __GEARMAN_H__

#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/uio.h>

#include <libgearman/constants.h>
#include <libgearman/types.h>
#include <libgearman/packet.h>
#include <libgearman/con.h>
#include <libgearman/job.h>
#include <libgearman/task.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * There is no locking within a single gearman_st structure, so for threaded
 * applications you must either ensure isolation in the application or use
 * multiple gearman_st structures (for example, one for each thread).
 */

/* Initialize a gearman structure. */
gearman_st *gearman_create(gearman_st *gearman);

/* Clone a gearman structure. */
gearman_st *gearman_clone(gearman_st *gearman, gearman_st *from);

/* Free a gearman structure. */
void gearman_free(gearman_st *gearman);

/* Reset state for a gearman structure. */
void gearman_reset(gearman_st *gearman);

/* Return an error string for last library error encountered. */
char *gearman_error(gearman_st *gearman);

/* Value of errno in the case of a GEARMAN_ERRNO return value. */
int gearman_errno(gearman_st *gearman);

/* Set options for a gearman structure. */
void gearman_set_options(gearman_st *gearman, gearman_options options,
                         uint32_t data);

/* Wait for I/O on connections. */
gearman_return gearman_io_wait(gearman_st *gearman, bool set_read);

/* Get next connection that is ready for I/O. */
gearman_con_st *gearman_io_ready(gearman_st *gearman);

/* Data structures. */
struct gearman_st
{
  gearman_options options;
  gearman_con_st *con_list;
  uint32_t con_count;
  gearman_job_st *job_list;
  uint32_t job_count;
  gearman_task_st *task_list;
  uint32_t task_count;
  gearman_packet_st *packet_list;
  uint32_t packet_count;
  struct pollfd *pfds;
  uint32_t pfds_size;
  gearman_con_st *con_ready;
  uint32_t sending;
  int last_errno;
  char last_error[GEARMAN_ERROR_SIZE];
};

/* These headers are at the end because they need gearman_st defined. */
#include <libgearman/client.h>
#include <libgearman/worker.h>

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_H__ */
