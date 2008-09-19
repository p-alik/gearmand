/*
 * Summary: Common include file for libgearman
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Brian Aker
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/un.h>
#include <netinet/tcp.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#define GEARMAN_INTERNAL 1

#include <libgearman/gearman.h>
#include <libgearman/io.h>

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
  GEAR_VERIFY_KEY= (1 << 10),
} gearman_flags;

/* Hashing algo */
void md5_signature(unsigned char *key, unsigned int length,
                   unsigned char *result);
uint32_t hash_crc32(const char *data, size_t data_len);
uint32_t hsieh_hash(char *key, size_t key_length);
uint32_t murmur_hash(char *key, size_t key_length);

gearman_return gearman_connect(gearman_server_st *ptr);
uint32_t find_server(gearman_st *ptr);
void gearman_quit_server(gearman_server_st *ptr, uint8_t io_death);

#define gearman_server_response_increment(A) (A)->cursor_active++
#define gearman_server_response_decrement(A) (A)->cursor_active--
#define gearman_server_response_reset(A) (A)->cursor_active=0

#endif /* __COMMON_H__ */
