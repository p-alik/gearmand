/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief System Include Files
 */

#ifndef __GEARMAN_SERVER_COMMON_H__
#define __GEARMAN_SERVER_COMMON_H__

#include "config.h"

#define GEARMAN_CORE
#include "gearmand.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STDDEF_H
#include <stddef.h>
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
#ifdef HAVE_STRINGS_H
#include <strings.h>
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

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#define likely(__x) if((__x))
#define unlikely(__x) if((__x))
#else
#define likely(__x) if(__builtin_expect((__x), 1))
#define unlikely(__x) if(__builtin_expect((__x), 0))
#endif

/**
 * Macro to print verbose messages.
 * @ingroup gearman_constants
 */

#define GEARMAN_LOG(__gearman, __verbose, ...) { \
  if ((__gearman)->log_fn != NULL) \
  { \
    char _log_buffer[GEARMAN_MAX_ERROR_SIZE]; \
    snprintf(_log_buffer, GEARMAN_MAX_ERROR_SIZE, __VA_ARGS__); \
    (*((__gearman)->log_fn))(_log_buffer, __verbose, \
                             (void *)(__gearman)->log_context); \
  } \
}

/**
 * Macro to log fatal errors.
 * @ingroup gearman_constants
 */
#define GEARMAN_FATAL(__gearman, ...) { \
  unlikely ((__gearman)->verbose >= GEARMAN_VERBOSE_FATAL) \
    GEARMAN_LOG(__gearman, GEARMAN_VERBOSE_FATAL, __VA_ARGS__) \
}
#define GEARMAN_SERVER_FATAL(__server, ...) \
  GEARMAN_FATAL((__server)->gearman, __VA_ARGS__)

/**
 * Macro to log errors.
 * @ingroup gearman_constants
 */
#define GEARMAN_ERROR(__gearman, ...) { \
  unlikely ((__gearman)->verbose >= GEARMAN_VERBOSE_ERROR) \
    GEARMAN_LOG(__gearman, GEARMAN_VERBOSE_ERROR, __VA_ARGS__) \
}
#define GEARMAN_SERVER_ERROR(__server, ...) \
  GEARMAN_ERROR((__server)->gearman, __VA_ARGS__)

/**
 * Macro to log infomational messages.
 * @ingroup gearman_constants
 */
#define GEARMAN_INFO(__gearman, ...) { \
  unlikely ((__gearman)->verbose >= GEARMAN_VERBOSE_INFO) \
    GEARMAN_LOG(__gearman, GEARMAN_VERBOSE_INFO, __VA_ARGS__) \
}
#define GEARMAN_SERVER_INFO(__server, ...) \
  GEARMAN_INFO((__server)->gearman, __VA_ARGS__)

/**
 * Macro to log errors.
 * @ingroup gearman_constants
 */
#define GEARMAN_DEBUG(__gearman, ...) { \
  unlikely ((__gearman)->verbose >= GEARMAN_VERBOSE_DEBUG) \
    GEARMAN_LOG(__gearman, GEARMAN_VERBOSE_DEBUG, __VA_ARGS__) \
}
#define GEARMAN_SERVER_DEBUG(__server, ...) \
  GEARMAN_DEBUG((__server)->gearman, __VA_ARGS__)

/**
 * Macro to log errors.
 * @ingroup gearman_constants
 */
#define GEARMAN_CRAZY(__gearman, ...) { \
  unlikely ((__gearman)->verbose >= GEARMAN_VERBOSE_CRAZY) \
    GEARMAN_LOG(__gearman, GEARMAN_VERBOSE_CRAZY, __VA_ARGS__) \
}
#define GEARMAN_SERVER_CRAZY(__server, ...) \
  GEARMAN_CRAZY((__server)->gearman, __VA_ARGS__)

/**
 * Macro to set error string.
 * @ingroup gearman_constants
 */
#define GEARMAN_ERROR_SET(__gearman, __function, ...) { \
  if ((__gearman)->log_fn == NULL) \
  { \
    snprintf((__gearman)->last_error, GEARMAN_MAX_ERROR_SIZE, \
             __function ":" __VA_ARGS__); \
  } \
  else \
    GEARMAN_FATAL(__gearman, __function ":" __VA_ARGS__) \
}
#define GEARMAN_SERVER_ERROR_SET(__server, ...) \
  GEARMAN_ERROR_SET((__server)->gearman, __VA_ARGS__)

/**
 * Macro to set error string.
 * @ingroup gearman_conf_constants
 */
#define GEARMAN_CONF_ERROR_SET(__conf, __function, ...) { \
  snprintf((__conf)->last_error, GEARMAN_MAX_ERROR_SIZE, \
           __function ":" __VA_ARGS__); \
}

/**
 * Lock only if we are multi-threaded.
 * @ingroup gearman_server_thread
 */
#define GEARMAN_SERVER_THREAD_LOCK(__thread) { \
  if ((__thread)->server->thread_count > 1) \
    (void) pthread_mutex_lock(&((__thread)->lock)); \
}

/**
 * Unlock only if we are multi-threaded.
 * @ingroup gearman_server_thread
 */
#define GEARMAN_SERVER_THREAD_UNLOCK(__thread) { \
  if ((__thread)->server->thread_count > 1) \
    (void) pthread_mutex_unlock(&((__thread)->lock)); \
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
  __list ## _count++; \
}

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
  __list ## _count--; \
}

/**
 * Add an object to a fifo list.
 * @ingroup gearman_constants
 */
#define GEARMAN_FIFO_ADD(__list, __obj, __prefix) { \
  if (__list ## _end == NULL) \
    __list ## _list= __obj; \
  else \
    __list ## _end->__prefix ## next= __obj; \
  __list ## _end= __obj; \
  __list ## _count++; \
}

/**
 * Delete an object from a fifo list.
 * @ingroup gearman_constants
 */
#define GEARMAN_FIFO_DEL(__list, __obj, __prefix) { \
  __list ## _list= __obj->__prefix ## next; \
  if (__list ## _list == NULL) \
    __list ## _end= NULL; \
  __list ## _count--; \
}

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
  __hash ## _count++; \
}

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
  __hash ## _count--; \
}

/* All thread-safe libevent functions are not in libevent 1.3x, and this is the
   common package version. Make this work for these earlier versions. */
#ifndef HAVE_EVENT_BASE_NEW
#define event_base_new event_init
#endif

#ifndef HAVE_EVENT_BASE_FREE
#define event_base_free (void)
#endif

#ifndef HAVE_EVENT_BASE_GET_METHOD
#define event_base_get_method(__base) event_get_method()
#endif

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_SERVER_COMMON_H__ */
