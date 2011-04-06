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

#ifndef __GEARMAN_COMMON_H__
#define __GEARMAN_COMMON_H__

#include <config.h>
#include <stdint.h>

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
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

#include <errno.h>
#include <limits.h>

#define BUILDING_LIBGEARMAN
#define GEARMAN_CORE

#include <libgearman/gearman.h>

/* These are private not to be installed headers */
#include <libgearman/byteorder.h>
#include <libgearman/function.h>

#endif /* __GEARMAN_COMMON_H__ */
