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

#ifdef __cplusplus
extern "C" {
#endif

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
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
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

/**
 * Macro to set error string.
 * @ingroup gearman_constants
 */
#define GEARMAN_ERROR_SET(__gearman, __function, ...) { \
  snprintf((__gearman)->last_error, GEARMAN_MAX_ERROR_SIZE, \
           __function ":" __VA_ARGS__); }

/**
 * Macro to set error string for gearmand.
 * @ingroup gearman_constants
 */
#define GEARMAND_ERROR_SET(__gearmand, __function, ...) { \
  snprintf((__gearmand)->last_error, GEARMAN_MAX_ERROR_SIZE, \
           __function ":" __VA_ARGS__); }

/**
 * Macro to print verbose messages.
 * @ingroup gearman_constants
 */
#define GEARMAND_VERBOSE(__gearmand, __level, ...) { \
  if ((__gearmand)->verbose > (__level)) \
  { \
    char _verbose_buffer[GEARMAN_MAX_ERROR_SIZE]; \
    snprintf(_verbose_buffer, GEARMAN_MAX_ERROR_SIZE, __VA_ARGS__); \
    printf("%s\n", _verbose_buffer); \
  } \
}

/**
 * Add an object to a list.
 * @ingroup gearman_constants
 */
#define GEARMAN_LIST_ADD(__list, __obj, __prefix) { \
  if (__list ## _list != NULL) \
    __list ## _list->__prefix ## prev= __obj; \
  __obj->__prefix ## next= __list ## _list; \
  __obj->__prefix ## prev= NULL; \
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
  __obj->__prefix ## prev= NULL; \
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

/* All thread-safe libevent functions are not in libevent 1.3x, and this is the
   common package version. Make this work for these earlier versions. */
#ifndef HAVE_EVENT_BASE_NEW
#define event_base_new event_init
#endif

#ifndef HAVE_EVENT_BASE_GET_METHOD
#define event_base_get_method(__base) event_get_method()
#endif

/**
 * Command information array.
 * @ingroup gearman_constants
 */
extern gearman_command_info_st gearman_command_info_list[GEARMAN_COMMAND_MAX];

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_COMMON_H__ */
