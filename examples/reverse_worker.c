/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Example Worker
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libgearman/gearman.h>

typedef enum
{
  REVERSE_WORKER_OPTIONS_NONE=   0,
  REVERSE_WORKER_OPTIONS_DATA=   (1 << 0),
  REVERSE_WORKER_OPTIONS_STATUS= (1 << 1),
  REVERSE_WORKER_OPTIONS_UNIQUE= (1 << 2)
} reverse_worker_options_t;

static void *reverse(gearman_job_st *job, void *context,
                     size_t *result_size, gearman_return_t *ret_ptr);

static void usage(char *name);

int main(int argc, char *argv[])
{
  int c;
  uint32_t count= 0;
  char *host= NULL;
  in_port_t port= 0;
  reverse_worker_options_t options= REVERSE_WORKER_OPTIONS_NONE;
  int timeout= -1;
  gearman_return_t ret;
  gearman_worker_st worker;

  while ((c = getopt(argc, argv, "c:dh:p:st:u")) != -1)
  {
    switch(c)
    {
    case 'c':
      count= (uint32_t)atoi(optarg);
      break;

    case 'd':
      options|= REVERSE_WORKER_OPTIONS_DATA;
      break;

    case 'h':
      host= optarg;
      break;

    case 'p':
      port= (in_port_t)atoi(optarg);
      break;

    case 's':
      options|= REVERSE_WORKER_OPTIONS_STATUS;
      break;

    case 't':
      timeout= atoi(optarg);
      break;

    case 'u':
      options|= REVERSE_WORKER_OPTIONS_UNIQUE;
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

  if (gearman_worker_create(&worker) == NULL)
  {
    fprintf(stderr, "Memory allocation failure on worker creation\n");
    exit(1);
  }

  if (options & REVERSE_WORKER_OPTIONS_UNIQUE)
    gearman_worker_add_options(&worker, GEARMAN_WORKER_GRAB_UNIQ);

  if (timeout >= 0)
    gearman_worker_set_timeout(&worker, timeout);

  ret= gearman_worker_add_server(&worker, host, port);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_worker_error(&worker));
    exit(1);
  }

  ret= gearman_worker_add_function(&worker, "reverse", 0, reverse,
                                   &options);
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

  return 0;
}

static void *reverse(gearman_job_st *job, void *context,
                     size_t *result_size, gearman_return_t *ret_ptr)
{
  reverse_worker_options_t options= *((reverse_worker_options_t *)context);
  const uint8_t *workload;
  uint8_t *result;
  size_t x;
  size_t y;

  workload= gearman_job_workload(job);
  *result_size= gearman_job_workload_size(job);

  result= malloc(*result_size);
  if (result == NULL)
  {
    fprintf(stderr, "malloc:%d\n", errno);
    *ret_ptr= GEARMAN_WORK_FAIL;
    return NULL;
  }

  for (y= 0, x= *result_size; x; x--, y++)
  {
    result[y]= ((uint8_t *)workload)[x - 1];

    if (options & REVERSE_WORKER_OPTIONS_DATA)
    {
      *ret_ptr= gearman_job_send_data(job, &(result[y]), 1);
      if (*ret_ptr != GEARMAN_SUCCESS)
      {
        free(result);
        return NULL;
      }
    }

    if (options & REVERSE_WORKER_OPTIONS_STATUS)
    {
      *ret_ptr= gearman_job_send_status(job, (uint32_t)y,
                                        (uint32_t)*result_size);
      if (*ret_ptr != GEARMAN_SUCCESS)
      {
        free(result);
        return NULL;
      }

      sleep(1);
    }
  }

  printf("Job=%s%s%s Workload=%.*s Result=%.*s\n", gearman_job_handle(job),
         options & REVERSE_WORKER_OPTIONS_UNIQUE ? " Unique=" : "",
         options & REVERSE_WORKER_OPTIONS_UNIQUE ? gearman_job_unique(job) : "",
         (int)*result_size, workload, (int)*result_size, result);

  *ret_ptr= GEARMAN_SUCCESS;

  if (options & REVERSE_WORKER_OPTIONS_DATA)
  {
    *result_size= 0;
    return NULL;
  }

  return result;
}

static void usage(char *name)
{
  printf("\nusage: %s [-h <host>] [-p <port>]\n", name);
  printf("\t-c <count>   - number of jobs to run before exiting\n");
  printf("\t-d           - send result back in data chunks\n");
  printf("\t-h <host>    - job server host\n");
  printf("\t-p <port>    - job server port\n");
  printf("\t-s           - send status updates and sleep while running job\n");
  printf("\t-t <timeout> - timeout in milliseconds\n");
  printf("\t-u           - when grabbing jobs, grab the uniqie id\n");
}
