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
#include <libgearman/structs.h>
#include <libgearman/con.h>
#include <libgearman/packet.h>
#include <libgearman/task.h>
#include <libgearman/job.h>
#include <libgearman/client.h>
#include <libgearman/worker.h>

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

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_H__ */
