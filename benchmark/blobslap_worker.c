/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Blob slap worker utility
 */

#include "benchmark.h"

static void *worker_fn(gearman_job_st *job, void *context,
                       size_t *result_size, gearman_return_t *ret_ptr);

static void usage(char *name);

int main(int argc, char *argv[])
{
  gearman_benchmark_st benchmark;
  int c;
  char *host= NULL;
  in_port_t port= 0;
  char *function= NULL;
  uint32_t count= 0;
  gearman_return_t ret;
  gearman_worker_st worker;

  benchmark_init(&benchmark);

  if (gearman_worker_create(&worker) == NULL)
  {
    fprintf(stderr, "Memory allocation failure on worker creation\n");
    exit(1);
  }

  while ((c = getopt(argc, argv, "c:f:h:p:v")) != -1)
  {
    switch(c)
    {
    case 'c':
      count= (uint32_t)atoi(optarg);
      break;

    case 'f':
      function= optarg;
      ret= gearman_worker_add_function(&worker, function, 0, worker_fn,
                                       &benchmark);
      if (ret != GEARMAN_SUCCESS)
      {
        fprintf(stderr, "%s\n", gearman_worker_error(&worker));
        exit(1);
      }
      break;

    case 'h':
      host= optarg;
      ret= gearman_worker_add_server(&worker, host, port);
      if (ret != GEARMAN_SUCCESS)
      {
        fprintf(stderr, "%s\n", gearman_worker_error(&worker));
        exit(1);
      }
      break;

    case 'p':
      port= (in_port_t)atoi(optarg);
      break;

    case 'v':
      benchmark.verbose++;
      break;

    default:
      usage(argv[0]);
      exit(1);
    }
  }

  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
  {
    fprintf(stderr, "signal:%d\n", errno);
    exit(1);
  }

  if (host == NULL)
  {
    ret= gearman_worker_add_server(&worker, NULL, port);
    if (ret != GEARMAN_SUCCESS)
    {
      fprintf(stderr, "%s\n", gearman_worker_error(&worker));
      exit(1);
    }
  }

  if (function == NULL)
  {
    ret= gearman_worker_add_function(&worker,
                                     GEARMAN_BENCHMARK_DEFAULT_FUNCTION, 0,
                                     worker_fn, &benchmark);
    if (ret != GEARMAN_SUCCESS)
    {
      fprintf(stderr, "%s\n", gearman_worker_error(&worker));
      exit(1);
    }
  }

  while (1)
  {
    ret= gearman_worker_work(&worker);
    if (ret != GEARMAN_SUCCESS)
    {
      fprintf(stderr, "%s\n", gearman_worker_error(&worker));
      break;
    }

    if (count > 0)
    {
      count--;
      if (count == 0)
        break;
    }
  }

  gearman_worker_free(&worker);

  return 0;
}

static void *worker_fn(gearman_job_st *job, void *context,
                       size_t *result_size, gearman_return_t *ret_ptr)
{
  gearman_benchmark_st *benchmark= (gearman_benchmark_st *)context;

  (void)result_size;

  if (benchmark->verbose > 0)
    benchmark_check_time(benchmark);

  if (benchmark->verbose > 1)
  {
    printf("Job=%s (%zu)\n", gearman_job_handle(job),
           gearman_job_workload_size(job));
  }

  *ret_ptr= GEARMAN_SUCCESS;
  return NULL;
}

static void usage(char *name)
{
  printf("\nusage: %s\n"
         "\t[-c count] [-f function] [-h <host>] [-p <port>] [-v]\n\n", name);
  printf("\t-c <count>    - number of jobs to run before exiting\n");
  printf("\t-f <function> - function name for tasks, can specify many\n"
         "\t                (default %s)\n",
                            GEARMAN_BENCHMARK_DEFAULT_FUNCTION);
  printf("\t-h <host>     - job server host, can specify many\n");
  printf("\t-p <port>     - job server port\n");
  printf("\t-v            - increase verbose level\n");
}
