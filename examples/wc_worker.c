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
#include <string.h>
#include <unistd.h>

#include <libgearman/gearman.h>

static void *wc(gearman_job_st *job, void *cb_arg, size_t *result_size,
                gearman_return_t *ret_ptr);

static void usage(char *name);

int main(int argc, char *argv[])
{
  char c;
  uint32_t count= 0;
  char *host= NULL;
  unsigned short port= 0;
  gearman_return_t ret;
  gearman_worker_st worker;

  while ((c = getopt(argc, argv, "c:h:p:")) != EOF)
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

  ret= gearman_worker_add_function(&worker, "wc", 0, wc, NULL);
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

static void *wc(gearman_job_st *job, void *cb_arg, size_t *result_size,
                gearman_return_t *ret_ptr)
{
  const uint8_t *workload;
  uint8_t *result;
  size_t x;
  uint64_t count= 0;
  (void)cb_arg;

  workload= gearman_job_workload(job);
  *result_size= gearman_job_workload_size(job);

  result= malloc(20);
  if (result == NULL)
  {
    fprintf(stderr, "malloc:%d\n", errno);
    *ret_ptr= GEARMAN_WORK_FAIL;
    return NULL;
  }

  if (workload != NULL)
  {
    if (workload[0] != ' ' && workload[0] != '\t' && workload[0] != '\n')
      count++;

    for (x= 0; x < *result_size; x++)
    {
      if (workload[x] != ' ' && workload[x] != '\t' && workload[x] != '\n')
        continue;

      count++;

      while (workload[x] == ' ' || workload[x] == '\t' || workload[x] == '\n')
      {
        x++;
        if (x == *result_size)
        {
          count--;
          break;
        }
      }
    }
  }

  snprintf(result, 20, "%" PRIu64, count);

  printf("Job=%s Workload=%.*s Result=%s\n", gearman_job_handle(job),
         (int)*result_size, workload, result);

  *result_size= strlen(result) + 1;

  *ret_ptr= GEARMAN_SUCCESS;
  return result;
}

static void usage(char *name)
{
  printf("\nusage: %s [-h <host>] [-p <port>]\n", name);
  printf("\t-h <host> - job server host\n");
  printf("\t-p <port> - job server port\n");
}
