/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief System include files
 */

#ifndef __GEARMAN_COMMON_H__
#define __GEARMAN_COMMON_H__

#include "config.h"

#include "gearman.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>
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

#ifdef HAVE_EVENT_H
#include <event.h>
#endif

/**
 * Macro to set error string.
 * @ingroup gearman_constants
 */
#define GEARMAN_ERROR_SET(__gearman, __function, ...) { \
  snprintf((__gearman)->last_error, GEARMAN_MAX_ERROR_SIZE, \
           __function ":" __VA_ARGS__); }

/**
 * Add an object to a list.
 * @ingroup gearman_constants
 */
#define GEARMAN_LIST_ADD(__list, __obj, __prefix) { \
  if (__list ## _list != NULL) \
    __list ## _list->__prefix ## prev= __obj; \
  __obj->__prefix ## next= __list ## _list; \
  __list ## _list= __obj; \
  __list ## _count++; }

/**
 * Delete an object from a list.
 * @ingroup gearman_constants
 */
#define GEARMAN_LIST_DEL(__list, __obj, __prefix) { \
  if (__list ## _list == __obj) \
    __list ## _list= __obj->__prefix ## next; \
  if (__obj->__prefix ## prev != NULL) \
    __obj->__prefix ## prev->__prefix ## next= __obj->__prefix ## next; \
  if (__obj->__prefix ## next != NULL) \
    __obj->__prefix ## next->__prefix ## prev= __obj->__prefix ## prev; \
  __list ## _count--; }

/**
 * Add an object to a hash.
 * @ingroup gearman_constants
 */
#define GEARMAN_HASH_ADD(__hash, __key, __obj, __prefix) { \
  if (__hash ## _hash[__key] != NULL) \
    __hash ## _hash[__key]->__prefix ## prev= __obj; \
  __obj->__prefix ## next= __hash ## _hash[__key]; \
  __hash ## _hash[__key]= __obj; \
  __hash ## _count++; }

/**
 * Delete an object from a hash.
 * @ingroup gearman_constants
 */
#define GEARMAN_HASH_DEL(__hash, __key, __obj, __prefix) { \
  if (__hash ## _hash[__key] == __obj) \
    __hash ## _hash[__key]= __obj->__prefix ## next; \
  if (__obj->__prefix ## prev != NULL) \
    __obj->__prefix ## prev->__prefix ## next= __obj->__prefix ## next; \
  if (__obj->__prefix ## next != NULL) \
    __obj->__prefix ## next->__prefix ## prev= __obj->__prefix ## prev; \
  __hash ## _count--; }

/**
 * Command information array.
 * @ingroup gearman_constants
 */
extern gearman_command_info_st gearman_command_info_list[GEARMAN_COMMAND_MAX];

/**
 * @ingroup gearmand
 */
struct gearmand
{
  in_port_t port;
  int backlog;
  uint8_t verbose;
  gearman_server_st server;
  int listen_fd;
  gearman_return_t ret;
  gearmand_con_st *dcon_list;
  uint32_t dcon_count;
  uint32_t dcon_total;
#ifdef HAVE_EVENT_H
  struct event_base *base;
  struct event listen_event;
#endif
};

/**
 * @ingroup gearmand
 */
struct gearmand_con
{
  gearmand_con_st *next;
  gearmand_con_st *prev;
  int fd;
  struct sockaddr_in sa;
  gearmand_st *gearmand;
  gearman_server_con_st server_con;
  gearman_con_st *con;
#ifdef HAVE_EVENT_H
  struct event event;
#endif
};

#endif /* __GEARMAN_COMMON_H__ */
