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

#pragma once

#include <libgearman-server/gearmand.h>
#include <libgearman-server/byteorder.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
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

#define gearmand_array_size(__object) (sizeof((__object)) / sizeof(*(__object)))

#ifdef __cplusplus
}
#endif

#ifdef NI_MAXHOST
#define GEARMAND_NI_MAXHOST NI_MAXHOST
#else
#define GEARMAND_NI_MAXHOST 1025
#endif

#ifdef NI_MAXSERV
#define GEARMAND_NI_MAXSERV NI_MAXSERV
#else
#define GEARMAND_NI_MAXSERV 32
#endif
