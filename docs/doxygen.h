/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Main page for Doxygen
 */

/**
@mainpage Gearman Library

http://gearman.org/

Gearman is, at the most basic level, a job queuing system. It can be
used to farm out work to other machines, dispatch function calls to
machines that are better suited to do work, to do work in parallel,
to load balance lots of function calls, or to call functions between
languages.

There are three components: clients, job servers, and workers. Clients
submit jobs to job servers, the job servers find an available worker,
the worker runs the job, and then the result optionally gets sent
back to the client. This package provides the C implementation of
all components.

One note on tasks vs jobs. A task is usually used in the context of
a client, and a job is usually used in the context of the server and
worker. A task can be a job or other client request such as getting
job status.

When using the C library for client and worker interfaces, be sure
to handle SIGPIPE. The library does not do this to be sure it does
not interfere with the calling applications signal handling. This
can be done with code such as:

@code

if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
{
  fprintf(stderr, "signal:%d\n", errno);
  exit(1);
}

@endcode

@anchor main_page_client
@section client Client

It is best to look at the example source code (in examples/) included
in this package for complete code and error handling. You are able
to use the client interface in one of two ways: to run a single task,
or to run tasks concurrently.

In either case, you first need to create a client structure and add
job servers to connect to. For example:

@code

gearman_client_st client;

gearman_client_create(&client);

gearman_client_add_server(&client, "127.0.0.1", 0);

@endcode

You can pass either of the host and port fields of
gearman_client_add_server() as NULL and 0 respectively to use the
default values.

@anchor main_page_client_single
@subsection client_single Single Client Interface

One you have the client object setup, you can run a single task with:

@code

result= gearman_client_do(&client, "function", "argument", strlen("argument"),
                          &result_size, &ret);

@endcode

This will contact the job server, request "function" to be run with
"argument" as the argument, and return the result of that function. The
size of the result is stored in result_size. The return code is stored
in "ret" and should always be checked.

@anchor main_page_client_concurrent
@subsection client_concurrent Concurrent Client Interface

If you need to run multiple tasks at the same time, you'll want to
use the concurrent interface. After the client setup, you will then
add tasks to be run. This just queues the task and does not actually
run it. You then call gearman_run_tasks() to submit and process the
jobs in parallel. When using the concurrent interface, you need to
specify callback functions for each action you may care about. For
example, if you want to know when a task completes, you would set a
gearman_complete_fn callback function to be run for each task.

@code

static gearman_return_t complete(gearman_task_st *task)
{
  printf("Completed: %s %.*s\n", gearman_task_job_handle(task),
         (int)gearman_task_data_size(task), (char *)gearman_task_data(task));
  return GEARMAN_SUCCESS;
}

static gearman_return_t fail(gearman_task_st *task)
{
  printf("Failed: %s\n", gearman_task_job_handle(task));
  return GEARMAN_SUCCESS;
}

gearman_client_add_task(&client, &task1, NULL, "function", "argument1",
                        strlen("argument1"), &ret);
gearman_client_add_task(&client, &task2, NULL, "function", "argument2",
                        strlen("argument2"), &ret);
gearman_client_set_complete_fn(&client, complete);
gearman_client_set_fail_fn(&client, fail);
gearman_client_run_tasks(&client);

@endcode

After adding two tasks, they are run in parallel and the complete()
callback is called when each is done (or fail() if the job failed).

@anchor main_page_worker
@section worker Worker

It is best to look at the example source code (in examples/) included
in this package for complete code and error handling. The worker
interface allows you to register functions along with a callback,
and then enter into a loop answering requests from a job server. You
first need to create a worker structure and add job servers to connect
to. For example:

@code

gearman_worker_st worker;

gearman_worker_create(&worker);

gearman_worker_add_server(&worker, "127.0.0.1", 0);

@endcode

You can pass either of the host and port fields of
gearman_worker_add_server() as NULL and 0 respectively to use the
default values.

Once you have the worker object setup, you then need to register
functions with the job server. For example:

@code

gearman_worker_add_function(&worker, "function", 0, function_callback, NULL);

@endcode

This notifies all job servers that this worker can perform "function",
and saves the pointer to the "function_callback" in the worker
structure for use in the worker loop. To enter the worker loop,
you would use:

@code

while (1) gearman_worker_work(&worker);

@endcode

This waits for jobs to be assigned from the job server, and calls
the callback functions associated with the job's function name. This
function also handles all result packet processing back to the job
server once the callback completes.

The last component of the worker is the callback function. This will look like:

@code

void *function_callback(gearman_job_st *job, void *cb_arg, size_t *result_size,
                        gearman_return_t *ret_ptr)
{
  char *result;

  result= strdup((char *)gearman_job_workload(job));
  *result_size= gearman_job_workload_size(job);

  *ret_ptr= GEARMAN_SUCCESS;
  return result;
}

@endcode

This worker function simply echos the given string back. The
gearman_job_workload() workload function returns the workload sent from
the client, and gearman_job_workload_size() returns the size of the
workload. The callback function returns the workload to be sent back to
the client, or NULL if there is not any. The result_size pointer must
be set to the size of the workload being returned. The cb_arg argument
is the same pointer that was passed into gearman_worker_add_function(),
and allows the calling application to pass data around. The ret_ptr
argument should be set to the return code of the worker function,
which is normally GEARMAN_SUCCESS.

See the examples/ directory for the full worker code examples.

*/
