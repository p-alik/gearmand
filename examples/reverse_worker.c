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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libgearman/gearman.h>

static void *reverse(gearman_job_st *job, void *cb_arg, size_t *result_size,
                     gearman_return_t *ret_ptr);

static void usage(char *name);

int main(int argc, char *argv[])
{
  char c;
  char *host= NULL;
  unsigned short port= 0;
  gearman_return_t ret;
  gearman_worker_st worker;

  while((c = getopt(argc, argv, "h:p:")) != EOF)
  {
    switch(c)
    {
    case 'h':
      host= optarg;
      break;

    case 'p':
      port= atoi(optarg);
      break;

    default:
      usage(argv[0]);
      exit(1);
    }
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

  ret= gearman_worker_add_function(&worker, "reverse", 0, reverse, NULL);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_worker_error(&worker));
    exit(1);
  }

  /* This while loop has no body. */
  while (gearman_worker_work(&worker) == GEARMAN_SUCCESS);
  fprintf(stderr, "%s\n", gearman_worker_error(&worker));

  gearman_worker_free(&worker);

  return 0;
}

static void *reverse(gearman_job_st *job, void *cb_arg, size_t *result_size,
                     gearman_return_t *ret_ptr)
{
  const uint8_t *workload;
  uint8_t *result;
  size_t x;
  size_t y;
  (void)cb_arg;

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
    result[y]= ((uint8_t *)workload)[x - 1];

  printf("Job=%s Workload=%.*s Result=%.*s\n", gearman_job_handle(job),
         (int)*result_size, workload, (int)*result_size, result);

  ret_ptr= GEARMAN_SUCCESS;
  return result;
}

static void usage(char *name)
{
  printf("\nusage: %s [-h <host>] [-p <port>]\n", name);
  printf("\t-h <host> - job server host\n");
  printf("\t-p <port> - job server port\n");
}
