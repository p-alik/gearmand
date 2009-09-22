/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Blob slap client utility
 */

#include "benchmark.h"

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
  gearman_benchmark_st benchmark;
  int c;
  char *host= NULL;
  in_port_t port= 0;
  const char *function= GEARMAN_BENCHMARK_DEFAULT_FUNCTION;
  uint32_t num_tasks= BLOBSLAP_DEFAULT_NUM_TASKS;
  size_t min_size= BLOBSLAP_DEFAULT_BLOB_MIN_SIZE;
  size_t max_size= BLOBSLAP_DEFAULT_BLOB_MAX_SIZE;
  uint32_t count= 1;
  gearman_return_t ret;
  gearman_client_st client;
  gearman_task_st *tasks;
  char *blob;
  size_t blob_size;
  uint32_t x;

  benchmark_init(&benchmark);

  if (gearman_client_create(&client) == NULL)
  {
    fprintf(stderr, "Memory allocation failure on client creation\n");
    exit(1);
  }

  gearman_client_add_options(&client, GEARMAN_CLIENT_UNBUFFERED_RESULT);

  while ((c= getopt(argc, argv, "bc:f:h:m:M:n:p:s:v")) != -1)
  {
    switch(c)
    {
    case 'b':
      benchmark.background= true;
      break;

    case 'c':
      count= (uint32_t)atoi(optarg);
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
      min_size= (size_t)atoi(optarg);
      break;

    case 'M':
      max_size= (size_t)atoi(optarg);
      break;

    case 'n':
      num_tasks= (uint32_t)atoi(optarg);
      break;

    case 'p':
      port= (in_port_t)atoi(optarg);
      break;

    case 's':
      srand((unsigned int)atoi(optarg));
      break;

    case 'v':
      benchmark.verbose++;
      break;

    default:
      gearman_client_free(&client);
      _usage(argv[0]);
      exit(1);
    }
  }

  if (host == NULL)
  {
    ret= gearman_client_add_server(&client, NULL, port);
    if (ret != GEARMAN_SUCCESS)
    {
      fprintf(stderr, "%s\n", gearman_client_error(&client));
      exit(1);
    }
  }

  if (min_size > max_size)
  {
    fprintf(stderr, "Min data size must be smaller than max data size\n");
    exit(1);
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
      if (min_size == max_size)
        blob_size= max_size;
      else
      {
        blob_size= (size_t)rand();

        if (max_size > RAND_MAX)
          blob_size*= (size_t)(rand() + 1);

        blob_size= (blob_size % (max_size - min_size)) + min_size;
      }

      if (benchmark.background)
      {
        (void)gearman_client_add_task_background(&client, &(tasks[x]),
                                                 &benchmark, function, NULL,
                                                 (void *)blob, blob_size, &ret);
      }
      else
      {
        (void)gearman_client_add_task(&client, &(tasks[x]), &benchmark,
                                      function, NULL, (void *)blob, blob_size,
                                      &ret);
      }

      if (ret != GEARMAN_SUCCESS)
      {
        fprintf(stderr, "%s\n", gearman_client_error(&client));
        exit(1);
      }
    }

    gearman_client_set_created_fn(&client, _created);
    gearman_client_set_data_fn(&client, _data);
    gearman_client_set_status_fn(&client, _status);
    gearman_client_set_complete_fn(&client, _complete);
    gearman_client_set_fail_fn(&client, _fail);
    ret= gearman_client_run_tasks(&client);
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
  gearman_benchmark_st *benchmark;

  benchmark= (gearman_benchmark_st *)gearman_task_context(task);

  if (benchmark->background && benchmark->verbose > 0)
    benchmark_check_time(benchmark);

  if (benchmark->verbose > 2)
    printf("Created: %s\n", gearman_task_job_handle(task));

  return GEARMAN_SUCCESS;
}

static gearman_return_t _status(gearman_task_st *task)
{
  gearman_benchmark_st *benchmark;

  benchmark= (gearman_benchmark_st *)gearman_task_context(task);

  if (benchmark->verbose > 2)
  {
    printf("Status: %s (%u/%u)\n", gearman_task_job_handle(task),
           gearman_task_numerator(task), gearman_task_denominator(task));
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t _data(gearman_task_st *task)
{
  gearman_benchmark_st *benchmark;
  char buffer[BLOBSLAP_BUFFER_SIZE];
  size_t size;
  gearman_return_t ret;

  benchmark= (gearman_benchmark_st *)gearman_task_context(task);

  while (1)
  {
    size= gearman_task_recv_data(task, buffer, BLOBSLAP_BUFFER_SIZE, &ret);
    if (ret != GEARMAN_SUCCESS)
      return ret;
    if (size == 0)
      break;
  }

  if (benchmark->verbose > 2)
  {
    printf("Data: %s %zu\n", gearman_task_job_handle(task),
           gearman_task_data_size(task));
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t _complete(gearman_task_st *task)
{
  gearman_benchmark_st *benchmark;
  char buffer[BLOBSLAP_BUFFER_SIZE];
  size_t size;
  gearman_return_t ret;

  benchmark= (gearman_benchmark_st *)gearman_task_context(task);

  while (1)
  {
    size= gearman_task_recv_data(task, buffer, BLOBSLAP_BUFFER_SIZE, &ret);
    if (ret != GEARMAN_SUCCESS)
      return ret;
    if (size == 0)
      break;
  }

  if (benchmark->verbose > 0)
    benchmark_check_time(benchmark);

  if (benchmark->verbose > 1)
  {
    printf("Completed: %s %zu\n", gearman_task_job_handle(task),
           gearman_task_data_size(task));
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t _fail(gearman_task_st *task)
{
  gearman_benchmark_st *benchmark;

  benchmark= (gearman_benchmark_st *)gearman_task_context(task);

  if (benchmark->verbose > 0)
    benchmark_check_time(benchmark);

  if (benchmark->verbose > 1)
    printf("Failed: %s\n", gearman_task_job_handle(task));

  return GEARMAN_SUCCESS;
}

static void _usage(char *name)
{
  printf("\nusage: %s\n"
         "\t[-c count] [-f <function>] [-h <host>] [-m <min_size>]\n"
         "\t[-M <max_size>] [-n <num_tasks>] [-p <port>] [-s] [-v]\n\n", name);
  printf("\t-c <count>     - number of times to run all tasks\n");
  printf("\t-f <function>  - function name for tasks (default %s)\n",
         GEARMAN_BENCHMARK_DEFAULT_FUNCTION);
  printf("\t-h <host>      - job server host, can specify many\n");
  printf("\t-m <min_size>  - minimum blob size (default %d)\n",
         BLOBSLAP_DEFAULT_BLOB_MIN_SIZE);
  printf("\t-M <max_size>  - maximum blob size (default %d)\n",
         BLOBSLAP_DEFAULT_BLOB_MAX_SIZE);
  printf("\t-n <num_tasks> - number of tasks to run at once (default %d)\n",
         BLOBSLAP_DEFAULT_NUM_TASKS);
  printf("\t-p <port>      - job server port\n");
  printf("\t-s <seed>      - seed random number for blobsize with <seed>\n");
  printf("\t-v            - increase verbose level\n");
}
