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

#ifndef __GEARMAN_COMMON_H__
#define __GEARMAN_COMMON_H__

#define GEARMAN_INTERNAL 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#include <libgearman/io.h>
#include <libgearman/quit.h>
#include <libgearman/constants.h>
#include <libgearman/types.h>
#include <libgearman/watchpoint.h>
#include <libgearman/str_error.h>
#include <libgearman/str_action.h>
#include <libgearman/byte_array.h>
#include <libgearman/result.h>
#include <libgearman/response.h>

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STRINGS_H
#include <string.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_STDLIB_H
#include <netdb.h>
#endif
#ifdef HAVE_STDLIB_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <limits.h>
#endif
#ifdef HAVE_STDLIB_H
#include <assert.h>
#endif
#ifdef HAVE_STDLIB_H
#include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
#include <poll.h>
#endif
#ifdef HAVE_STDLIB_H
#include <fcntl.h>
#endif
#ifdef HAVE_STDLIB_H
#include <sys/un.h>
#endif
#ifdef HAVE_STDLIB_H
#include <netinet/tcp.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#define __builtin_expect(x, expected_value) (x)

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#else

#define likely(x)       if((x))
#define unlikely(x)     if((x))

#endif

#define GEARMAN_BLOCK_SIZE 1024
#define GEARMAN_DEFAULT_COMMAND_SIZE 350
#define TINY_STRING_LEN 128
#define SMALL_STRING_LEN 1024
#define HUGE_STRING_LEN 8192
#define PACKET_HEADER_LENGTH 12

#define gearman_server_response_increment(A) (A)->cursor_active++
#define gearman_server_response_decrement(A) (A)->cursor_active--
#define gearman_server_response_reset(A) (A)->cursor_active=0

typedef enum {
  GEAR_NO_BLOCK= (1 << 0),
  GEAR_TCP_NODELAY= (1 << 1),
  GEAR_REUSE_GEARORY= (1 << 2),
  GEAR_USE_CACHE_LOOKUPS= (1 << 6),
} gearman_flags;


#endif /* __GEARMAN_COMMON_H__ */
