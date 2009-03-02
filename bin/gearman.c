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

typedef struct
{
  char *host;
  in_port_t port;
  int32_t count;
  char *unique;
  bool job_per_newline;
  bool strip_newline;
  bool worker;
  bool suppress_input;
  char **argv;
} gearman_args_st;

void _client(gearman_args_st *args);

void _client_run(gearman_client_st *client, gearman_args_st *args,
                 const void *workload, size_t workload_size);

void _worker(gearman_args_st *args);

static void *_worker_cb(gearman_job_st *job, void *cb_arg, size_t *result_size,
                        gearman_return_t *ret_ptr);

void _read_workload(int fd, char **workload, size_t *workload_offset,
                    size_t *workload_size);

static void usage(char *name);

int main(int argc, char *argv[])
{
  char c;
  gearman_args_st args;

  memset(&args, 0, sizeof(gearman_args_st));

  while ((c = getopt(argc, argv, "c:h:nNp:su:w")) != EOF)
  {
    switch(c)
    {
    case 'c':
      args.count= atoi(optarg);
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

    default:
      usage(argv[0]);
      exit(1);
    }
  }

  if (argc <= optind)
  {
    usage(argv[0]);
    exit(1);
  }

  args.argv= argv + optind;

  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
  {
    fprintf(stderr, "signal:%d\n", errno);
    exit(1);
  }

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

  if (gearman_client_create(&client) == NULL)
  {
    fprintf(stderr, "Memory allocation failure on client creation\n");
    exit(1);
  }

  ret= gearman_client_add_server(&client, args->host, args->port);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_client_error(&client));
    exit(1);
  }

  if (args->suppress_input)
    _client_run(&client, args, NULL, 0);
  else if (args->job_per_newline)
  {
    workload= malloc(GEARMAN_INITIAL_WORKLOAD_SIZE);
    if (workload == NULL)
    {
      fprintf(stderr, "Memory allocation failure on buffer creation\n");
      exit(1);
    }

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

  gearman_client_free(&client);

  if (workload != NULL)
    free(workload);
}

void _client_run(gearman_client_st *client, gearman_args_st *args,
                 const void *workload, size_t workload_size)
{
  char **function;
  gearman_return_t ret;
  char *result;
  size_t result_size;
  uint32_t numerator;
  uint32_t denominator;
  ssize_t write_ret;

  for (function= args->argv; *function != NULL; function++)
  {
    while (1)
    {
      result= (char *)gearman_client_do(client, *function, args->unique,
                                        workload, workload_size, &result_size,
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
        gearman_client_do_status(client, &numerator, &denominator);
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
        fprintf(stderr, "%s\n", gearman_client_error(client));

      break;
    }
  }
}

void _worker(gearman_args_st *args)
{
  gearman_worker_st worker;
  gearman_return_t ret;
  
  if (gearman_worker_create(&worker) == NULL)
  {
    fprintf(stderr, "Memory allocation failure on worker creation\n");
    exit(1);
  }

  ret= gearman_worker_add_server(&worker, args->host, args->port);
  if (ret != GEARMAN_SUCCESS)
  {
    fprintf(stderr, "%s\n", gearman_worker_error(&worker));
    exit(1);
  }

  ret= gearman_worker_add_function(&worker, args->argv[0], 0, _worker_cb, args);
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
  ssize_t write_ret;
  int in_fds[2];
  int out_fds[2];
  char *result= NULL;
  size_t total_size= 0;

  if (args->argv[1] == NULL)
  {
    write_ret= write(1, gearman_job_workload(job),
                     gearman_job_workload_size(job));
    if (write_ret < 0)
    {
      fprintf(stderr, "Error writing to standard output (%d)\n", errno);
      exit(1);
    }
  }
  else
  {
    if (pipe(in_fds) < 0 || pipe(out_fds) < 0)
    {
      fprintf(stderr, "Error creating pipes (%d)\n", errno);
      exit(1);
    }

    switch (fork())
    {
    case -1:
      fprintf(stderr, "Error forking process (%d)\n", errno);
      exit(1);

    case 0:
      if (dup2(in_fds[0], 0) < 0)
      {
        fprintf(stderr, "Error in dup2 (%d)\n", errno);
        exit(1);
      }

      close(in_fds[1]);

      if (dup2(out_fds[1], 1) < 0)
      {
        fprintf(stderr, "Error in 2dup2 (%d)\n", errno);
        exit(1);
      }

      close(out_fds[0]);

      execvp(args->argv[1], args->argv + 1);
      fprintf(stderr, "Error running execvp (%d)\n", errno);
      exit(1);

    default:
      break;
    }

    close(in_fds[0]);
    close(out_fds[1]);

    if (gearman_job_workload_size(job) > 0)
    {
      write_ret= write(in_fds[1], gearman_job_workload(job),
                       gearman_job_workload_size(job));
      if (write_ret < 0)
      {
        fprintf(stderr, "Error writing to process (%d)\n", errno);
        exit(1);
      }
    }

    close(in_fds[1]);
    
    _read_workload(out_fds[0], &result, result_size, &total_size);

    close(out_fds[0]);
  }

  *ret_ptr= GEARMAN_SUCCESS;
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
      {
        fprintf(stderr, "Memory allocation failure on workload\n");
        exit(1);
      }
    }

    read_ret= read(fd, *workload + *workload_offset,
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
  printf("\nusage: %s [client or worker options]\n\n", name);
  printf("gearman [-h <host>] [-p <port>] [-u <unique>] <function> ...\n");
  printf("gearman -w [-h <host>] [-p <port>] <function> "
         "[<command> [<args> ...]]\n\n");
  printf("\t-c <count>  - number of jobs for worker to run before exiting\n");
  printf("\t-h <host>   - job server host\n");
  printf("\t-n          - send one job per line\n");
  printf("\t-N          - send one job per line, stripping off newline\n");
  printf("\t-p <port>   - job server port\n");
  printf("\t-s          - send job without reading from standard input\n");
  printf("\t-u <unique> - unique key to use for job\n");
  printf("\t-w          - run as a worker\n");
}
