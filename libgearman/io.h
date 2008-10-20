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

#ifndef __LIBGEARMAN_IO_H__
#define __LIBGEARMAN_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <libgearman/libgearman_config.h>
#include <libgearman/constants.h>
#include <libgearman/types.h>
#include <libgearman/watchpoint.h>
#include <libgearman/server.h>

ssize_t gearman_io_write(gearman_server_st *ptr, char *buffer, size_t length,
                         char with_flush);
void gearman_io_reset(gearman_server_st *ptr);
ssize_t gearman_io_read(gearman_server_st *ptr, char *buffer, size_t length);
gearman_return gearman_io_close(gearman_server_st *ptr);

#ifdef __cplusplus
}
#endif

#endif /* __LIBGEARMAN_IO_H__ */
