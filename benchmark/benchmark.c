/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Common benchmark functions
 */

#include "benchmark.h"

/*
 * Public definitions
 */

void benchmark_init(gearman_benchmark_st *benchmark)
{
  memset(benchmark, 0, sizeof(gearman_benchmark_st));
  gettimeofday(&(benchmark->total), NULL);
  gettimeofday(&(benchmark->begin), NULL);
}

void benchmark_check_time(gearman_benchmark_st *benchmark)
{
  benchmark->jobs++;

  gettimeofday(&(benchmark->end), NULL);
  if (benchmark->end.tv_sec != benchmark->begin.tv_sec)
  {
    benchmark->total_jobs+= benchmark->jobs;

    printf("[Current: %6"PRIu64" jobs/s, Total: %6"PRIu64" jobs/s]\n",
           ((uint64_t)(benchmark->jobs) * 1000000) /
           ((((uint64_t)(benchmark->end.tv_sec) * 1000000) +
              (uint64_t)(benchmark->end.tv_usec)) -
            (((uint64_t)(benchmark->begin.tv_sec) * 1000000) +
              (uint64_t)(benchmark->begin.tv_usec))),
           ((uint64_t)(benchmark->total_jobs) * 1000000) /
           ((((uint64_t)(benchmark->end.tv_sec) * 1000000) +
              (uint64_t)(benchmark->end.tv_usec)) -
            (((uint64_t)(benchmark->total.tv_sec) * 1000000) +
              (uint64_t)(benchmark->total.tv_usec))));

    benchmark->jobs= 0;
    gettimeofday(&(benchmark->begin), NULL);
  }
}
