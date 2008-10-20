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

#include <libgearman/gearman.h>
#include <libgearman/connect.h>
#include <libgearman/io.h>

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

typedef enum {
  GEAR_NO_BLOCK= (1 << 0),
  GEAR_TCP_NODELAY= (1 << 1),
  GEAR_REUSE_GEARORY= (1 << 2),
  GEAR_USE_MD5= (1 << 3),
  GEAR_USE_KETAMA= (1 << 4),
  GEAR_USE_CRC= (1 << 5),
  GEAR_USE_CACHE_LOOKUPS= (1 << 6),
  GEAR_SUPPORT_CAS= (1 << 7),
  GEAR_BUFFER_REQUESTS= (1 << 8),
  GEAR_USE_SORT_HOSTS= (1 << 9),
  GEAR_VERIFY_KEY= (1 << 10)
} gearman_flags;

/* Hashing algo */
void md5_signature(unsigned char *key, unsigned int length,
                   unsigned char *result);
uint32_t hash_crc32(const char *data, size_t data_len);
uint32_t hsieh_hash(char *key, size_t key_length);
uint32_t murmur_hash(char *key, size_t key_length);

gearman_return gearman_connect(gearman_server_st *ptr);
uint32_t find_server(gearman_st *ptr);

#define gearman_server_response_increment(A) (A)->cursor_active++
#define gearman_server_response_decrement(A) (A)->cursor_active--
#define gearman_server_response_reset(A) (A)->cursor_active=0

#endif /* __GEARMAN_COMMON_H__ */
