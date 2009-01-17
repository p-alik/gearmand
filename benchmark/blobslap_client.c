/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Blob Slap Benchmark Utility
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>

#include <libgearman/gearman.h>

#define BLOBSLAP_DEFAULT_FUNCTION "blobslap"
#define BLOBSLAP_DEFAULT_NUM_TASKS 10
#define BLOBSLAP_DEFAULT_BLOB_MIN_SIZE 0
#define BLOBSLAP_DEFAULT_BLOB_MAX_SIZE 1024
#define BLOBSLAP_BUFFER_SIZE 8192

static gearman_return_t _created(gearman_task_st *task);
static gearman_return_t _data(gearman_task_st *task);
static gearman_return_t _status(gearman_task_st *task);
static gearman_return_t _complete(gearman_task_st *task);
static gearman_return_t _fail(gearman_task_st *task);

static void _usage(char *name);

int main(int argc, char *argv[])
{
  char c;
  char *host= NULL;
  in_port_t port= 0;
  char *function= BLOBSLAP_DEFAULT_FUNCTION;
  uint32_t num_tasks= BLOBSLAP_DEFAULT_NUM_TASKS;
  size_t min_size= BLOBSLAP_DEFAULT_BLOB_MIN_SIZE;
  size_t max_size= BLOBSLAP_DEFAULT_BLOB_MAX_SIZE;
  uint8_t verbose= 0;
  uint32_t count= 1;
  gearman_return_t ret;
  gearman_client_st client;
  gearman_task_st *tasks;
  char *blob;
  size_t blob_size;
  uint32_t x;

  if (gearman_client_create(&client) == NULL)
  {
    fprintf(stderr, "Memory allocation failure on client creation\n");
    exit(1);
  }

  while ((c= getopt(argc, argv, "c:f:h:m:M:n:p:s:v")) != EOF)
  {
    switch(c)
    {
    case 'c':
      count= atoi(optarg);
      break;

    case 'f':
      function= optarg;
      break;

    case 'h':
      host= optarg;
      ret= gearman_client_add_server(&client, host, port);
      if (ret != GEARMAN_SUCCESS)
      {
        fprintf(stderr, "%s\n", gearman_client_error(&client));
        exit(1);
      }
      break;

    case 'm':
      min_size= atoi(optarg);
      break;

    case 'M':
      max_size= atoi(optarg);
      break;

    case 'n':
      num_tasks= atoi(optarg);
      break;

    case 'p':
      port= atoi(optarg);
      break;

    case 's':
      srand(atoi(optarg));
      break;

    case 'v':
      verbose++;
      break;

    default:
      gearman_client_free(&client);
      _usage(argv[0]);
      exit(1);
    }
  }

  if (host == NULL)
  {
    ret= gearman_client_add_server(&client, host, port);
    if (ret != GEARMAN_SUCCESS)
    {
      fprintf(stderr, "%s\n", gearman_client_error(&client));
      exit(1);
    }
  }

  if (num_tasks == 0)
  {
    fprintf(stderr, "Number of tasks must be larger than zero\n");
    exit(1);
  }

  tasks= malloc(num_tasks * sizeof(gearman_task_st));
  if (tasks == NULL)
  {
    fprintf(stderr, "Memory allocation failure on malloc\n");
    exit(1);
  }
  
  blob= malloc(max_size);
  if (blob == NULL)
  {
    fprintf(stderr, "Memory allocation failure on malloc\n");
    exit(1);
  }

  memset(blob, 'x', max_size); 

  while (1)
  {
    for (x= 0; x < num_tasks; x++)
    {
      blob_size= rand();

      if (max_size > RAND_MAX)
        blob_size*= (rand() + 1);

      blob_size= (blob_size % (max_size - min_size)) + min_size;

      if (gearman_client_add_task(&client, &(tasks[x]), &verbose, function,
                                  NULL, (void *)blob, blob_size,
                                  &ret) == NULL || ret != GEARMAN_SUCCESS)
      {
        fprintf(stderr, "%s\n", gearman_client_error(&client));
        exit(1);
      }
    }

    ret= gearman_client_run_tasks(&client, NULL, _created, _data, _status,
                                  _complete, _fail);
    if (ret != GEARMAN_SUCCESS)
    {
      fprintf(stderr, "%s\n", gearman_client_error(&client));
      exit(1);
    }

    for (x= 0; x < num_tasks; x++)
      gearman_task_free(&(tasks[x]));

    if (count > 0)
    {
      count--;
      if (count == 0)
        break;
    }
  }

  free(blob);
  free(tasks);
  gearman_client_free(&client);

  return 0;
}

static gearman_return_t _created(gearman_task_st *task)
{
  uint8_t verbose= *((uint8_t *)gearman_task_fn_arg(task));

  if (verbose > 1)
    printf("Created: %s\n", gearman_task_job_handle(task));

  return GEARMAN_SUCCESS;
}

static gearman_return_t _status(gearman_task_st *task)
{
  uint8_t verbose= *((uint8_t *)gearman_task_fn_arg(task));

  if (verbose > 1)
  {
    printf("Status: %s (%u/%u)\n", gearman_task_job_handle(task),
           gearman_task_numerator(task), gearman_task_denominator(task));
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t _data(gearman_task_st *task)
{
  uint8_t verbose= *((uint8_t *)gearman_task_fn_arg(task));
  char buffer[BLOBSLAP_BUFFER_SIZE];
  size_t size;
  gearman_return_t ret;

  while (1)
  {
    size= gearman_task_recv_data(task, buffer, BLOBSLAP_BUFFER_SIZE, &ret);
    if (ret != GEARMAN_SUCCESS)
      return ret;
    if (size == 0)
      break;
  }

  if (verbose > 0)
  {
    printf("Data: %s %zu\n", gearman_task_job_handle(task),
           gearman_task_data_size(task));
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t _complete(gearman_task_st *task)
{
  uint8_t verbose= *((uint8_t *)gearman_task_fn_arg(task));
  char buffer[BLOBSLAP_BUFFER_SIZE];
  size_t size;
  gearman_return_t ret;

  while (1)
  {
    size= gearman_task_recv_data(task, buffer, BLOBSLAP_BUFFER_SIZE, &ret);
    if (ret != GEARMAN_SUCCESS)
      return ret;
    if (size == 0)
      break;
  }

  if (verbose > 0)
  {
    printf("Completed: %s %zu\n", gearman_task_job_handle(task),
           gearman_task_data_size(task));
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t _fail(gearman_task_st *task)
{
  uint8_t verbose= *((uint8_t *)gearman_task_fn_arg(task));

  if (verbose > 0)
    printf("Failed: %s\n", gearman_task_job_handle(task));

  return GEARMAN_SUCCESS;
}

static void _usage(char *name)
{
  printf("\nusage: %s\n\t[-f <function>] [-h <host>] [-m <min_size>]\n"
         "\t[-M <max_size>] [-n <num_tasks>] [-p <port>] [-s] [-v]\n\n", name);
  printf("\t-f <function>  - function name for tasks (default %s)\n",
         BLOBSLAP_DEFAULT_FUNCTION);
  printf("\t-h <host>      - job server host, can specify many\n");
  printf("\t-m <min_size>  - minimum blob size (default %d)\n",
         BLOBSLAP_DEFAULT_BLOB_MIN_SIZE);
  printf("\t-M <max_size>  - maximum blob size (default %d)\n",
         BLOBSLAP_DEFAULT_BLOB_MAX_SIZE);
  printf("\t-n <num_tasks> - number of tasks to run at once (default %d)\n",
         BLOBSLAP_DEFAULT_NUM_TASKS);
  printf("\t-p <port>      - job server port\n");
  printf("\t-s <seed>      - seed random number for blobsize with <seed>\n");
  printf("\t-v             - print verbose messages\n");
}
