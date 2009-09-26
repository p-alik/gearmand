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

/**
 * Utility function used for error logging
 * @ingroup gearman_private
 *
 * @param[in] gearman Structure previously initialized with gearman_create() or
 *  gearman_clone().
 * @param[in] function Name of function the error happened in. 
 * @param[in] format Format and variable argument list of message.
 */
GEARMAN_LOCAL
void gearman_error_set(gearman_st *gearman, const char *function,
                       const char *format, ...);

/**
 * Utility function used for parsing server lists.
 * @ingroup gearman_private
 */
GEARMAN_LOCAL
gearman_return_t gearman_parse_servers(const char *servers,
                                       gearman_parse_server_fn *callback,
                                       const void *context);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_COMMON_H__ */
