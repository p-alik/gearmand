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

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <vector>


#include <libgearman/gearman.h>

#include "bin/arguments.h"
#include "util/pidfile.h"
#include "util/error.h"

using namespace gearman_client;
using namespace gearman_util;

#define GEARMAN_INITIAL_WORKLOAD_SIZE 8192

struct worker_argument_t
{
  Args &args;
  Function &function;

  worker_argument_t(Args &args_arg, Function &function_arg) :
    args(args_arg),
    function(function_arg)
  {
  }
};

/**
 * Function to run in client mode.
 */
static void _client(Args &args);

/**
 * Run client jobs.
 */
static void _client_run(gearman_client_st *client, Args &args,
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
static void _worker(Args &args);

/**
 * Callback function when worker gets a job.
 */
static void *_worker_cb(gearman_job_st *job, void *context,
                        size_t *result_size, gearman_return_t *ret_ptr);

/**
 * Read workload chunk from a file descriptor and put into allocated memory.
 */
static void _read_workload(int fd, Bytes workload, size_t *workload_offset);

/**
 * Print usage information.
 */
static void usage(char *name);

extern "C"
{

static void signal_setup()
{
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
  {
    error::perror("signal");
  }
}

}

class Client
{
public:
  Client()
  {
    if (gearman_client_create(&_client) == NULL)
    {
      std::cerr << "Failed memory allocation while initializing client." << std::endl;
      abort();
    }
  }

  ~Client()
  {
    gearman_client_free(&_client);
  }

  gearman_client_st &client()
  {
    return _client;
  }

private:
  gearman_client_st _client;
};

class Worker
{
public:
  Worker()
  {
    if (gearman_worker_create(&_worker) == NULL)
    {
      std::cerr << "Failed memory allocation while initializing memory." << std::endl;
      abort();
    }
  }

  ~Worker()
  {
    gearman_worker_free(&_worker);
  }

  gearman_worker_st &worker()
  {
    return _worker;
  }

private:
  gearman_worker_st _worker;
};

int main(int argc, char *argv[])
{
  Args args(argc, argv);
  bool close_stdio= false;

  if (args.usage())
  {
    usage(argv[0]);
    exit(0);
  }

  signal_setup();

  if (args.daemon())
  {
    switch (fork())
    {
    case -1:
      fprintf(stderr, "gearmand: fork:%d\n", errno);
      return 1;

    case 0:
      break;

    default:
      return 0;
    }

    if (setsid() == -1)
    {
      fprintf(stderr, "gearmand: setsid:%d\n", errno);
      return 1;
    }

    close_stdio= true;
  }

  if (close_stdio)
  {
    int fd;

    /* If we can't remap stdio, it should not a fatal error. */
    fd= open("/dev/null", O_RDWR, 0);

    if (fd != -1)
    {
      if (dup2(fd, STDIN_FILENO) == -1)
      {
        fprintf(stderr, "gearmand: dup2:%d\n", errno);
        return 1;
      }

      if (dup2(fd, STDOUT_FILENO) == -1)
      {
        fprintf(stderr, "gearmand: dup2:%d\n", errno);
        return 1;
      }

      if (dup2(fd, STDERR_FILENO) == -1)
      {
        fprintf(stderr, "gearmand: dup2:%d\n", errno);
        return 1;
      }

      close(fd);
    }
  }

  Pidfile _pid_file(args.pid_file());

  if (not _pid_file.create())
  {
    error::perror(_pid_file.error_message().c_str());
    return 1;
  }

  if (args.worker())
  {
    _worker(args);
  }
  else
  {
    _client(args);
  }

  return args.error();
}

void _client(Args &args)
{
  Client local_client;
  gearman_client_st &client= local_client.client();
  gearman_return_t ret;
  Bytes workload;
  size_t workload_offset= 0;

  if (args.timeout() >= 0)
  {
    gearman_client_set_timeout(&client, args.timeout());
  }

  ret= gearman_client_add_server(&client, args.host(), args.port());
  if (ret != GEARMAN_SUCCESS)
  {
    error::message("gearman_client_add_server", client);
  }

  gearman_client_set_data_fn(&client, _client_data);
  gearman_client_set_warning_fn(&client, _client_warning);
  gearman_client_set_status_fn(&client, _client_status);
  gearman_client_set_complete_fn(&client, _client_data);
  gearman_client_set_exception_fn(&client, _client_warning);
  gearman_client_set_fail_fn(&client, _client_fail);

  if (not args.arguments())
  {
    if (args.suppress_input())
    {
      _client_run(&client, args, NULL, 0);
    }
    else if (args.job_per_newline())
    {
      workload.resize(GEARMAN_INITIAL_WORKLOAD_SIZE);

      while (1)
      {
        if (fgets(&workload[0], static_cast<int>(workload.size()), stdin) == NULL)
          break;

        if (args.strip_newline())
          _client_run(&client, args, &workload[0], strlen(&workload[0]) - 1);
        else
          _client_run(&client, args, &workload[0], strlen(&workload[0]));
      }
    }
    else
    {
      _read_workload(0, workload, &workload_offset);
      _client_run(&client, args, &workload[0], workload_offset);
    }
  }
  else
  {
    for (size_t x= 0; args.argument(x) != NULL; x++)
    {
      _client_run(&client, args, args.argument(x), strlen(args.argument(x)));
    }
  }
}

void _client_run(gearman_client_st *client, Args &args,
                 const void *workload, size_t workload_size)
{
  gearman_return_t ret;

  for (Function::vector::iterator iter= args.begin(); 
       iter != args.end();
       iter++)
  {
    Function &function= *iter;

    /* This is a bit nasty, but all we have currently is multiple function
       calls. */
    if (args.background())
    {
      switch (args.priority())
      {
      case GEARMAN_JOB_PRIORITY_HIGH:
        (void)gearman_client_add_task_high_background(client,
                                                      function.task(),
                                                      &args,
                                                      function.name(),
                                                      args.unique(),
                                                      workload,
                                                      workload_size, &ret);
        break;

      case GEARMAN_JOB_PRIORITY_NORMAL:
        (void)gearman_client_add_task_background(client,
                                                 function.task(),
                                                 &args,
                                                 function.name(),
                                                 args.unique(),
                                                 workload,
                                                 workload_size, &ret);
        break;

      case GEARMAN_JOB_PRIORITY_LOW:
        (void)gearman_client_add_task_low_background(client,
                                                     function.task(),
                                                     &args,
                                                     function.name(),
                                                     args.unique(),
                                                     workload,
                                                     workload_size, &ret);
        break;

      case GEARMAN_JOB_PRIORITY_MAX:
      default:
        /* This should never happen. */
        ret= GEARMAN_UNKNOWN_STATE;
        break;
      }
    }
    else
    {
      switch (args.priority())
      {
      case GEARMAN_JOB_PRIORITY_HIGH:
        (void)gearman_client_add_task_high(client,
                                           function.task(),
                                           &args,
                                           function.name(),
                                           args.unique(),
                                           workload, workload_size, &ret);
        break;

      case GEARMAN_JOB_PRIORITY_NORMAL:
        (void)gearman_client_add_task(client,
                                      function.task(),
                                      &args,
                                      function.name(),
                                      args.unique(),
                                      workload,
                                      workload_size, &ret);
        break;

      case GEARMAN_JOB_PRIORITY_LOW:
        (void)gearman_client_add_task_low(client,
                                          function.task(),
                                          &args,
                                          function.name(),
                                          args.unique(),
                                          workload, workload_size, &ret);
        break;

      case GEARMAN_JOB_PRIORITY_MAX:
      default:
        /* This should never happen. */
        ret= GEARMAN_UNKNOWN_STATE;
        break;
      }
    }

    if (ret != GEARMAN_SUCCESS)
    {
      error::message("gearman_client_add_task", *client);
    }
  }

  ret= gearman_client_run_tasks(client);
  if (ret != GEARMAN_SUCCESS)
  {
    error::message("gearman_client_run_tasks", *client);
  }
}

static gearman_return_t _client_data(gearman_task_st *task)
{
  const Args *args;

  args= static_cast<const Args*>(gearman_task_context(task));
  if (args->prefix())
  {
    fprintf(stdout, "%s: ", gearman_task_function_name(task));
    fflush(stdout);
  }

  if (write(fileno(stdout), gearman_task_data(task), gearman_task_data_size(task)) == -1)
  {
    error::perror("write");
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t _client_warning(gearman_task_st *task)
{
  const Args *args;

  args= static_cast<const Args*>(gearman_task_context(task));
  if (args->prefix())
  {
    fprintf(stderr, "%s: ", gearman_task_function_name(task));
    fflush(stderr);
  }

  if (write(fileno(stderr), gearman_task_data(task), gearman_task_data_size(task)) == -1)
  {
    error::perror("write");
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t _client_status(gearman_task_st *task)
{
  const Args *args;

  args= static_cast<const Args*>(gearman_task_context(task));
  if (args->prefix())
    printf("%s: ", gearman_task_function_name(task));

  printf("%u%% Complete\n", (gearman_task_numerator(task) * 100) /
         gearman_task_denominator(task));

  return GEARMAN_SUCCESS;
}

static gearman_return_t _client_fail(gearman_task_st *task)
{
  const Args *args;

  args= static_cast<const Args *>(gearman_task_context(task));
  if (args->prefix())
    fprintf(stderr, "%s: ", gearman_task_function_name(task));

  fprintf(stderr, "Job failed\n");

  args->set_error();

  return GEARMAN_SUCCESS;
}

void _worker(Args &args)
{
  gearman_return_t ret;
  Worker local_worker;
  gearman_worker_st &worker= local_worker.worker();

  if (args.timeout() >= 0)
    gearman_worker_set_timeout(&worker, args.timeout());

  ret= gearman_worker_add_server(&worker, args.host(), args.port());
  if (ret != GEARMAN_SUCCESS)
  {
    error::message("gearman_worker_add_server", worker);
  }

  for (Function::vector::iterator iter= args.begin(); 
       iter != args.end();
       iter++)
  {
    Function &function= *iter;
    worker_argument_t pass(args, *iter);
    ret= gearman_worker_add_function(&worker, function.name(), 0, _worker_cb, &pass);
    if (ret != GEARMAN_SUCCESS)
    {
      error::message("gearman_worker_add_function", worker);
    }
  }

  while (1)
  {
    ret= gearman_worker_work(&worker);
    if (ret != GEARMAN_SUCCESS)
    {
      error::message("gearman_worker_work", worker);
    }

    if (args.count() > 0)
    {
      --args.count();
      if (args.count() == 0)
        break;
    }
  }
}

extern "C" {
static bool local_wexitstatus(int status)
{
  if (WEXITSTATUS(status) != 0)
    return true;

  return false;
}
}

static void *_worker_cb(gearman_job_st *job, void *context,
                        size_t *result_size, gearman_return_t *ret_ptr)
{
  worker_argument_t *arguments= static_cast<worker_argument_t *>(context);
  int in_fds[2];
  int out_fds[2];
  FILE *f;
  int status;

  Args &args= arguments->args;
  Function &function= arguments->function;

  *ret_ptr= GEARMAN_SUCCESS;

  if (not args.arguments())
  {
    if (write(1, gearman_job_workload(job),
              gearman_job_workload_size(job)) == -1)
    {
      error::perror("write");
    }
  }
  else
  {
    if (pipe(in_fds) == -1 || pipe(out_fds) == -1)
    {
      error::perror("pipe");
    }

    switch (fork())
    {
    case -1:
      error::perror("fork");

    case 0:
      if (dup2(in_fds[0], 0) == -1)
      {
        error::perror("dup2");
      }

      close(in_fds[1]);

      if (dup2(out_fds[1], 1) == -1)
      {
        error::perror("dup2");
      }

      close(out_fds[0]);

      execvp(args.argument(0), args.argumentv());
      {
        error::perror("execvp");
      }

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
        error::perror("write");
      }
    }

    close(in_fds[1]);

    if (args.job_per_newline())
    {
      f= fdopen(out_fds[0], "r");
      if (f == NULL)
      {
        error::perror("fdopen");
      }

      function.buffer().clear();
      function.buffer().resize(GEARMAN_INITIAL_WORKLOAD_SIZE);

      while (1)
      {
        if (fgets(function.buffer_ptr(), GEARMAN_INITIAL_WORKLOAD_SIZE, f) == NULL)
          break;

        if (args.strip_newline())
        {
          *ret_ptr= gearman_job_send_data(job, function.buffer_ptr(), strlen(function.buffer_ptr()) - 1);
        }
        else
        {
          *ret_ptr= gearman_job_send_data(job, function.buffer_ptr(), strlen(function.buffer_ptr()));
        }

        if (*ret_ptr != GEARMAN_SUCCESS)
          break;
      }

      function.buffer().clear();
      fclose(f);
    }
    else
    {
      _read_workload(out_fds[0], function.buffer(), result_size);
      close(out_fds[0]);
    }

    if (wait(&status) == -1)
    {
      error::perror("wait");
    }

    if (local_wexitstatus(status))
    {
      if (not function.buffer().empty())
      {
        *ret_ptr= gearman_job_send_data(job, function.buffer_ptr(), *result_size);
        if (*ret_ptr != GEARMAN_SUCCESS)
          return NULL;
      }

      *ret_ptr= GEARMAN_WORK_FAIL;
      return NULL;
    }
  }

  return function.buffer_ptr();
}

void _read_workload(int fd, Bytes workload, size_t *workload_offset)
{
  ssize_t read_ret;
  size_t workload_size= 0;

  while (1)
  {
    if (*workload_offset == workload_size)
    {
      if (workload_size == 0)
      {
        workload_size= GEARMAN_INITIAL_WORKLOAD_SIZE;
      }
      else
      {
        workload_size= workload_size * 2;
      }

      workload.resize(workload_size);
    }

    read_ret= read(fd, &workload[0] + *workload_offset,
                   workload_size - *workload_offset);

    if (read_ret == -1)
    {
      error::perror("execvp");
    }
    else if (read_ret == 0)
    {
      break;
    }

    *workload_offset += static_cast<size_t>(read_ret);
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
  printf("\t-t <timeout>  - Timeout in milliseconds\n");
  printf("\t-i <pidfile>  - Create a pidfile for the process\n");

  printf("\nClient options:\n");
  printf("\t-b            - Run jobs in the background\n");
  printf("\t-I            - Run jobs as high priority\n");
  printf("\t-L            - Run jobs as low priority\n");
  printf("\t-n            - Run one job per line\n");
  printf("\t-N            - Same as -n, but strip off the newline\n");
  printf("\t-P            - Prefix all output lines with functions names\n");
  printf("\t-s            - Send job without reading from standard input\n");
  printf("\t-u <unique>   - Unique key to use for job\n");

  printf("\nWorker options:\n");
  printf("\t-c <count>    - Number of jobs for worker to run before exiting\n");
  printf("\t-n            - Send data packet for each line\n");
  printf("\t-N            - Same as -n, but strip off the newline\n");
  printf("\t-w            - Run in worker mode\n");
}
