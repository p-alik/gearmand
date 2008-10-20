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

#ifndef __GEARMAN_DISPATCH_H__
#define __GEARMAN_DISPATCH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <libgearman/libgearman_config.h>
#include <libgearman/constants.h>
#include <libgearman/types.h>
#include <libgearman/watchpoint.h>
#include <libgearman/server.h>

gearman_return gearman_dispatch(gearman_server_st *server, 
                                gearman_action action,
                                giov_st *giov,
                                bool with_flush);

/* IO Vector */
struct giov_st {
  const void *arg;
  ssize_t arg_length;
};

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_DISPATCH_H__ */
