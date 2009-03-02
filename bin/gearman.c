/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman Command Line Tool
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libgearman/gearman.h>

#define GEARMAN_INITIAL_WORKLOAD_SIZE 8192

void _client(int argc, char *argv[], const char *host, in_port_t port,
             const char *unique);

void _worker(int argc, char *argv[], const char *host, in_port_t port,
             int32_t count);

static void *_worker_cb(gearman_job_st *job, void *cb_arg, size_t *result_size,
                        gearman_return_t *ret_ptr);

void _read_workload(char **workload, size_t *workload_offset,
                    size_t *workload_size);

static void usage(char *name);

int main(int argc, char *argv[])
{
  char c;
  char *host= NULL;
  in_port_t port= 0;
  int32_t count= 0;
  char *unique= NULL;
  bool worker= false;

  while ((c = getopt(argc, argv, "c:h:p:u:w")) != EOF)
  {
    switch(c)
    {
    case 'c':
      count= atoi(optarg);
      break;

    case 'h':
      host= optarg;
      break;

    case 'p':
      port= atoi(optarg);
      break;

    case 'u':
      unique= optarg;
      break;

    case 'w':
      worker= true;
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

  if (worker)
    _worker(argc, argv, host, port, count);
  else
    _client(argc, argv, host, port, unique);

  return 0;
}

void _client(int argc, char *argv[], const char *host, in_port_t port,
             const char *unique)
{
  char *workload= NULL;
  size_t workload_size= 0;
  size_t workload_offset= 0;
  ssize_t write_ret;
  gearman_return_t ret;
  gearman_client_st client;
  char *result;
  size_t result_size;
  uint32_t numerator;
  uint32_t denominator;

  if (argc != (optind + 1))
  {
    usage(argv[0]);
    exit(1);
  }

  _read_workload(&workload, &workload_offset, &workload_size);

  if (gearman_client_create(&client) == NULL)
  {
    fprintf(stderr, "Memory allocation failure on client creation\n");
    exit(1);
  }

  ret= gearman_client_add_server(&client, host, port);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_client_error(&client));
    exit(1);
  }

  while (1)
  {
    result= (char *)gearman_client_do(&client, argv[optind], unique,
                                      workload, workload_offset, &result_size,
                                      &ret);
    if (ret == GEARMAN_WORK_DATA)
    {
      write(1, result, result_size);
      if (write_ret < 0)
      {
        fprintf(stderr, "Error writing to standard output (%d)\n", errno);
        exit(1);
      }

      free(result);
      continue;
    }
    else if (ret == GEARMAN_WORK_STATUS)
    {
      gearman_client_do_status(&client, &numerator, &denominator);
      printf("%u%% Complete\n", (numerator * 100) / denominator);
      continue;
    }
    else if (ret == GEARMAN_SUCCESS)
    {
      write(1, result, result_size);
      free(result);
    }
    else if (ret == GEARMAN_WORK_FAIL)
      fprintf(stderr, "Job failed\n");
    else
      fprintf(stderr, "%s\n", gearman_client_error(&client));

    break;
  }

  gearman_client_free(&client);

  if (workload != NULL)
    free(workload);
}

void _worker(int argc, char *argv[], const char *host, in_port_t port,
             int32_t count)
{
  gearman_worker_st worker;
  gearman_return_t ret;

  if (argc < (optind + 1))
  {
    usage(argv[0]);
    exit(1);
  }

  if (gearman_worker_create(&worker) == NULL)
  {
    fprintf(stderr, "Memory allocation failure on worker creation\n");
    exit(1);
  }

  ret= gearman_worker_add_server(&worker, host, port);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_worker_error(&worker));
    exit(1);
  }

  ret= gearman_worker_add_function(&worker, argv[optind], 0, _worker_cb, NULL);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_worker_error(&worker));
    exit(1);
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
}

static void *_worker_cb(gearman_job_st *job, void *cb_arg, size_t *result_size,
                        gearman_return_t *ret_ptr)
{
  ssize_t write_ret;
  (void) result_size;
  (void) cb_arg;

  write_ret= write(1, gearman_job_workload(job),
                   gearman_job_workload_size(job));
  if (write_ret < 0)
  {
    fprintf(stderr, "Error writing to standard output (%d)\n", errno);
    exit(1);
  }

  *ret_ptr= GEARMAN_SUCCESS;
  return NULL;
}

void _read_workload(char **workload, size_t *workload_offset,
                    size_t *workload_size)
{
  ssize_t read_ret;

  while (1)
  {
    if (*workload_offset == *workload_size)
    {
      if (*workload_size == 0)
        *workload_size= GEARMAN_INITIAL_WORKLOAD_SIZE;
      else
        *workload_size= *workload_size * 2;

      *workload= realloc(*workload, *workload_size);
      if(*workload == NULL)
      {
        fprintf(stderr, "Memory allocation failure on workload\n");
        exit(1);
      }
    }

    read_ret= read(0, *workload + *workload_offset,
                   *workload_size - *workload_offset);
    if (read_ret < 0)
    {
      fprintf(stderr, "Error reading from standard input (%d)\n", errno);
      exit(1);
    }
    else if (read_ret == 0)
      break;

    *workload_offset += read_ret;
  }
}

static void usage(char *name)
{
  printf("\nusage: %s [-h <host>] [-p <port>] [-u <unique>] <function>\n",
         name);
  printf("\nusage: %s -w [-h <host>] [-p <port>] <function> [cmd [args ...]]\n",
         name);
  printf("\t-c <count>  - number of jobs for worker to run before exiting\n");
  printf("\t-h <host>   - job server host\n");
  printf("\t-p <port>   - job server port\n");
  printf("\t-u <unique> - unique key to use for job\n");
  printf("\t-w          - run as a worker\n");
}
