/* Module configuration library
 * Copyright (C) 2009 Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief System include files
 */

#ifndef __MODCONF_COMMON_H__
#define __MODCONF_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "modconf.h"

#include "config.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#if 0
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if 0
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
#endif

/**
 * Macro to set error string.
 * @ingroup modconf_constants
 */
#define MODCONF_ERROR_SET(__modconf, __function, ...) { \
  snprintf((__modconf)->last_error, MODCONF_MAX_ERROR_SIZE, \
           __function ":" __VA_ARGS__); \
}

#ifdef __cplusplus
}
#endif

#endif /* __MODCONF_COMMON_H__ */
