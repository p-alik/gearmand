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

#define GEARMAN_ERROR(...) \
{ \
  fprintf(stderr, "gearman:" __VA_ARGS__); \
  fprintf(stderr, "\n"); \
  exit(1); \
}

/**
 * Data structure for arguments and state.
 */
typedef struct
{
  char **function;
  uint32_t function_count;
  char *host;
  in_port_t port;
  uint32_t count;
  char *unique;
  bool job_per_newline;
  bool strip_newline;
  bool worker;
  bool suppress_input;
  char **argv;
  gearman_task_st *task;
} gearman_args_st;

/**
 * Function to run in client mode.
 */
static void _client(gearman_args_st *args);

/**
 * Run client jobs.
 */
static void _client_run(gearman_client_st *client, gearman_args_st *args,
                        const void *workload, size_t workload_size);

/**
 * Client data/complete callback function.
 */
static gearman_return_t _client_data(gearman_task_st *task);

/**
 * Client warning/exception callback function.
 */
static gearman_return_t _client_warning(gearman_task_st *task);

/**
 * Client status callback function.
 */
static gearman_return_t _client_status(gearman_task_st *task);
/**
 * Client fail callback function.
 */
static gearman_return_t _client_fail(gearman_task_st *task);

/**
 * Function to run in worker mode.
 */
static void _worker(gearman_args_st *args);

/**
 * Callback function when worker gets a job.
 */
static void *_worker_cb(gearman_job_st *job, void *cb_arg, size_t *result_size,
                        gearman_return_t *ret_ptr);

/**
 * Read workload chunk from a file descriptor and put into allocated memory.
 */
static void _read_workload(int fd, char **workload, size_t *workload_offset,
                           size_t *workload_size);

/**
 * Print usage information.
 */
static void usage(char *name);

int main(int argc, char *argv[])
{
  char c;
  gearman_args_st args;

  memset(&args, 0, sizeof(gearman_args_st));

  /* Allocate the maximum number of possible functions. */
  args.function= malloc(sizeof(char *) * argc);
  if (args.function == NULL)
    GEARMAN_ERROR("malloc:%d", errno)

  while ((c = getopt(argc, argv, "c:f:h:HnNp:su:w")) != EOF)
  {
    switch(c)
    {
    case 'c':
      args.count= atoi(optarg);
      break;

    case 'f':
      args.function[args.function_count]= optarg;
      args.function_count++;
      break;

    case 'h':
      args.host= optarg;
      break;

    case 'n':
      args.job_per_newline= true;
      break;

    case 'N':
      args.job_per_newline= true;
      args.strip_newline= true;
      break;

    case 'p':
      args.port= atoi(optarg);
      break;

    case 's':
      args.suppress_input= true;
      break;

    case 'u':
      args.unique= optarg;
      break;

    case 'w':
      args.worker= true;
      break;

    case 'H':
    default:
      usage(argv[0]);
      exit(1);
    }
  }

  args.argv= argv + optind;

  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    GEARMAN_ERROR("signal:%d", errno)

  if (args.worker)
    _worker(&args);
  else
    _client(&args);

  return 0;
}

void _client(gearman_args_st *args)
{
  gearman_client_st client;
  gearman_return_t ret;
  char *workload= NULL;
  size_t workload_size= 0;
  size_t workload_offset= 0;
  uint32_t x;

  if (gearman_client_create(&client) == NULL)
    GEARMAN_ERROR("Memory allocation failure on client creation")

  ret= gearman_client_add_server(&client, args->host, args->port);
  if (ret != GEARMAN_SUCCESS)
    GEARMAN_ERROR("gearman_client_add_server:%s", gearman_client_error(&client))

  gearman_client_set_data_fn(&client, _client_data);
  gearman_client_set_warning_fn(&client, _client_warning);
  gearman_client_set_status_fn(&client, _client_status);
  gearman_client_set_complete_fn(&client, _client_data);
  gearman_client_set_exception_fn(&client, _client_warning);
  gearman_client_set_fail_fn(&client, _client_fail);

  args->task= malloc(sizeof(gearman_task_st) * args->function_count);
  if (args->task == NULL)
    GEARMAN_ERROR("malloc:%d", errno)

  if (args->argv[0] == NULL)
  {
    if (args->suppress_input)
      _client_run(&client, args, NULL, 0);
    else if (args->job_per_newline)
    {
      workload= malloc(GEARMAN_INITIAL_WORKLOAD_SIZE);
      if (workload == NULL)
        GEARMAN_ERROR("malloc:%d", errno)

      while (1)
      {
        if (fgets(workload, GEARMAN_INITIAL_WORKLOAD_SIZE, stdin) == NULL)
          break;

        if (args->strip_newline)
          _client_run(&client, args, workload, strlen(workload) - 1);
        else
          _client_run(&client, args, workload, strlen(workload));
      }
    }
    else
    {
      _read_workload(0, &workload, &workload_offset, &workload_size);
      _client_run(&client, args, workload, workload_offset);
    }
  }
  else
  {
    for (x= 0; args->argv[x] != NULL; x++)
      _client_run(&client, args, args->argv[x], strlen(args->argv[x]));
  }

  gearman_client_free(&client);

  if (workload != NULL)
    free(workload);
}

void _client_run(gearman_client_st *client, gearman_args_st *args,
                 const void *workload, size_t workload_size)
{
  gearman_return_t ret;
  uint32_t x;

  for (x= 0; x < args->function_count; x++)
  {
    (void)gearman_client_add_task(client, &(args->task[x]), args,
                                  args->function[x], args->unique, workload,
                                  workload_size, &ret);
    if (ret != GEARMAN_SUCCESS)
      GEARMAN_ERROR("gearman_client_add_task:%s", gearman_client_error(client))
  }

  ret= gearman_client_run_tasks(client);
  if (ret != GEARMAN_SUCCESS)
    GEARMAN_ERROR("gearman_client_run_tasks:%s", gearman_client_error(client))

  for (x= 0; x < args->function_count; x++)
    gearman_task_free(&(args->task[x]));
}

static gearman_return_t _client_data(gearman_task_st *task)
{
  if (write(1, gearman_task_data(task), gearman_task_data_size(task)) == -1)
    GEARMAN_ERROR("write:%d", errno)

  return GEARMAN_SUCCESS;
}

static gearman_return_t _client_warning(gearman_task_st *task)
{
  if (write(2, gearman_task_data(task), gearman_task_data_size(task)) == -1)
    GEARMAN_ERROR("write:%d", errno)

  return GEARMAN_SUCCESS;
}

static gearman_return_t _client_status(gearman_task_st *task)
{
  printf("%u%% Complete\n", (gearman_task_numerator(task) * 100) /
         gearman_task_denominator(task));

  return GEARMAN_SUCCESS;
}

static gearman_return_t _client_fail(gearman_task_st *task)
{
  (void) task;

  fprintf(stderr, "Job failed\n");

  return GEARMAN_SUCCESS;
}

void _worker(gearman_args_st *args)
{
  gearman_worker_st worker;
  gearman_return_t ret;
  uint32_t x;
  
  if (gearman_worker_create(&worker) == NULL)
    GEARMAN_ERROR("Memory allocation failure on client creation")

  ret= gearman_worker_add_server(&worker, args->host, args->port);
  if (ret != GEARMAN_SUCCESS)
    GEARMAN_ERROR("gearman_worker_add_server:%s", gearman_worker_error(&worker))

  for (x= 0; x < args->function_count; x++)
  {
    ret= gearman_worker_add_function(&worker, args->function[x], 0, _worker_cb,
                                     args);
    if (ret != GEARMAN_SUCCESS)
    {
      GEARMAN_ERROR("gearman_worker_add_function:%s",
                    gearman_worker_error(&worker))
    }
  }

  while (1)
  {
    ret= gearman_worker_work(&worker);
    if (ret != GEARMAN_SUCCESS)
      GEARMAN_ERROR("gearman_worker_work:%s", gearman_worker_error(&worker))

    if (args->count > 0)
    {
      args->count--;
      if (args->count == 0)
        break;
    }
  }

  gearman_worker_free(&worker);
}

static void *_worker_cb(gearman_job_st *job, void *cb_arg, size_t *result_size,
                        gearman_return_t *ret_ptr)
{
  gearman_args_st *args= (gearman_args_st *)cb_arg;
  int in_fds[2];
  int out_fds[2];
  char *result= NULL;
  size_t total_size= 0;
  FILE *f;

  *ret_ptr= GEARMAN_SUCCESS;

  if (args->argv[0] == NULL)
  {
    if (write(1, gearman_job_workload(job),
              gearman_job_workload_size(job)) == -1)
    {
      GEARMAN_ERROR("write:%d", errno)
    }
  }
  else
  {
    if (pipe(in_fds) == -1 || pipe(out_fds) == -1)
      GEARMAN_ERROR("pipe:%d", errno)

    switch (fork())
    {
    case -1:
      GEARMAN_ERROR("fork:%d", errno)

    case 0:
      if (dup2(in_fds[0], 0) == -1)
        GEARMAN_ERROR("dup2:%d", errno)

      close(in_fds[1]);

      if (dup2(out_fds[1], 1) == -1)
        GEARMAN_ERROR("dup2:%d", errno)

      close(out_fds[0]);

      execvp(args->argv[0], args->argv);
      GEARMAN_ERROR("execvp:%d", errno)

    default:
      break;
    }

    close(in_fds[0]);
    close(out_fds[1]);

    if (gearman_job_workload_size(job) > 0)
    {
      if (write(in_fds[1], gearman_job_workload(job),
                gearman_job_workload_size(job)) == -1)
      {
        GEARMAN_ERROR("write:%d", errno)
      }
    }

    close(in_fds[1]);
    
    if (args->job_per_newline)
    {
      f= fdopen(out_fds[0], "r");
      if (f == NULL)
        GEARMAN_ERROR("fdopen:%d", errno)

      result= malloc(GEARMAN_INITIAL_WORKLOAD_SIZE);
      if (result == NULL)
        GEARMAN_ERROR("malloc:%d", errno)

      while (1)
      {
        if (fgets(result, GEARMAN_INITIAL_WORKLOAD_SIZE, f) == NULL)
          break;

        if (args->strip_newline)
          *ret_ptr= gearman_job_data(job, result, strlen(result) - 1);
        else
          *ret_ptr= gearman_job_data(job, result, strlen(result));

        if (*ret_ptr != GEARMAN_SUCCESS)
          break;
      }

      free(result);
      result= NULL;
      fclose(f);
    }
    else
    {
      _read_workload(out_fds[0], &result, result_size, &total_size);
      close(out_fds[0]);
    }
  }

  return result;
}

void _read_workload(int fd, char **workload, size_t *workload_offset,
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
        GEARMAN_ERROR("realloc:%d", errno)
    }

    read_ret= read(fd, *workload + *workload_offset,
                   *workload_size - *workload_offset);
    if (read_ret == -1)
      GEARMAN_ERROR("execvp:%d", errno)
    else if (read_ret == 0)
      break;

    *workload_offset += read_ret;
  }
}

static void usage(char *name)
{
  printf("Client mode: %s [options] [<data>]\n", name);
  printf("Worker mode: %s -w [options] [<command> [<args> ...]]\n", name);

  printf("\nCommon options to both client and worker modes.\n");
  printf("\t-f <function> - Function name to use for jobs (can give many)\n");
  printf("\t-h <host>     - Job server host\n");
  printf("\t-H            - Print this help menu\n");
  printf("\t-p <port>     - Job server port\n");

  printf("\nClient options:\n");
  printf("\t-n            - Run one job per line\n");
  printf("\t-N            - Same as -n, but strip off the newline\n");
  printf("\t-s            - Send job without reading from standard input\n");
  printf("\t-u <unique>   - Unique key to use for job\n");

  printf("\nWorker options:\n");
  printf("\t-c <count>    - Number of jobs for worker to run before exiting\n");
  printf("\t-n            - Send data packet for each line\n");
  printf("\t-N            - Same as -n, but strip off the newline\n");
  printf("\t-w            - Run in worker mode\n");
}
