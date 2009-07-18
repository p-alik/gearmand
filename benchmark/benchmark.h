/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Common benchmark header
 */

#ifndef __GEARMAN_BENCHMARK_H__
#define __GEARMAN_BENCHMARK_H__

#include "config.h"

#include <libgearman/gearman.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
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

#define GEARMAN_BENCHMARK_DEFAULT_FUNCTION "gb"

/**
 * @addtogroup benchmark Common Benchmark Utilities
 * @{
 */
typedef struct
{
  bool background;
  uint8_t verbose;
  uint64_t total_jobs;
  uint64_t jobs;
  struct timeval total;
  struct timeval begin;
  struct timeval end;
} gearman_benchmark_st;

/**
 * Initialize a benchmark strucutre.
 */
void benchmark_init(gearman_benchmark_st *benchmark);

/**
 * Check and possibly print time.
 */
void benchmark_check_time(gearman_benchmark_st *benchmark);

/** @} */

#endif /* __GEARMAN_BENCHMARK_H__ */
