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

#include <benchmark/benchmark.h>
#include <cerrno>
#include <cstdio>
#include <climits>
#include <iostream>
#include "util/daemon.h"

static void *worker_fn(gearman_job_st *job, void *context,
                       size_t *result_size, gearman_return_t *ret_ptr);

static void usage(char *name);


static gearman_return_t shutdown_fn(gearman_job_st*, void* /* context */)
{
  return GEARMAN_SHUTDOWN;
}


int main(int argc, char *argv[])
{
  gearman_benchmark_st benchmark;
  int c;
  char *host= NULL;
  in_port_t port= 0;
  char *function= NULL;
  bool opt_daemon= false;
  unsigned long int count= ULONG_MAX;
  gearman_worker_st worker;

  if (not gearman_worker_create(&worker))
  {
    std::cerr << "Failed to allocate worker" << std::endl;
    exit(EXIT_FAILURE);
  }

  while ((c = getopt(argc, argv, "dc:f:h:p:v")) != -1)
  {
    switch(c)
    {
    case 'c':
      count= strtoul(optarg, NULL, 10);
      break;

    case 'f':
      function= optarg;
      if (gearman_failed(gearman_worker_add_function(&worker, function, 0, worker_fn, &benchmark)))
      {
        std::cerr << "Failed adding function " << optarg << "() :" << gearman_worker_error(&worker) << std::endl;
        exit(EXIT_FAILURE);
      }
      break;

    case 'h':
      host= optarg;
      if (gearman_failed(gearman_worker_add_server(&worker, host, port)))
      {
        std::cerr << "Failed while adding server " << host << ":" << port << " :" << gearman_worker_error(&worker) << std::endl;
        exit(EXIT_FAILURE);
      }
      break;

    case 'p':
      port= in_port_t(atoi(optarg));
      break;

    case 'd':
      opt_daemon= true;
      break;

    case 'v':
      benchmark.verbose++;
      break;

    default:
      usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if (opt_daemon)
  {
    gearmand::daemonize(false, true);
  }

  if (opt_daemon)
    gearmand::daemon_is_ready(benchmark.verbose == 0);

  if (not host)
  {
    if (gearman_failed(gearman_worker_add_server(&worker, NULL, port)))
    {
      std::cerr << "Failing to add localhost:" << port << " :" << gearman_worker_error(&worker) << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  if (not function)
  {
    if (gearman_failed(gearman_worker_add_function(&worker,
                                                   GEARMAN_BENCHMARK_DEFAULT_FUNCTION, 0,
                                                   worker_fn, &benchmark)))
    {
      std::cerr << "Failed to add default function: " << gearman_worker_error(&worker) << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  gearman_function_t shutdown_function= gearman_function_create(shutdown_fn);
  if (gearman_failed(gearman_worker_define_function(&worker,
						    gearman_literal_param("shutdown"), 
						    shutdown_function,
						    0, 0)))
  {
    std::cerr << "Failed to add default function: " << gearman_worker_error(&worker) << std::endl;
    exit(EXIT_FAILURE);
  }

  do
  {
    gearman_return_t rc= gearman_worker_work(&worker);

    if (rc == GEARMAN_SHUTDOWN)
    {
      if (benchmark.verbose > 0)
        std::cerr << "shutdown" << std::endl;
      break;
    }
    else if (gearman_failed(rc))
    {
      std::cerr << "gearman_worker_work(): " << gearman_worker_error(&worker) << std::endl;
      break;
    }

    count--;
  } while(count);

  gearman_worker_free(&worker);

  return 0;
}

static void *worker_fn(gearman_job_st *job, void *context,
                       size_t *, gearman_return_t *ret_ptr)
{
  gearman_benchmark_st *benchmark= static_cast<gearman_benchmark_st *>(context);

  if (benchmark->verbose > 0)
    benchmark_check_time(benchmark);

  if (benchmark->verbose > 1)
  {
    std::cout << "Job=%s (" << gearman_job_workload_size(job) << ")" << std::endl;
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
