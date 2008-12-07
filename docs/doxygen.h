/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file
 * @brief Main page for Doxygen
 */

/**
@mainpage Gearman Library

Gearman is, at the most basic level, a job queuing system. It can be
used to farm out work to other machines, dispatc function calls to
machines that are better suited to do work, to do work in parallel,
to load balance lots of function calls, or to call functions between
languages.

There are three components: client, job server, and workers. Clients
submit jobs to job servers, the job servers find an available worker,
the worker runs the job, and then the result gets sent back to
the client.

This package provides the C implementation of all components.

One note on tasks vs jobs. A task is usually used in the context of
a client, and a job is usually used in the context of the server
and worker. A task can be a job or other client request such as
getting status.

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

gearman_client_add_server(&client, "127.0.0.1", 7003);

@endcode

You can pass either of the host and port fields of
gearman_client_add_server() as NULL and 0 respectively to use the
default values.

@subsection client_single Single Client Interface

One you have the client object setup, you can run a single task with:

@code
result= gearman_client_do(&client, "function", "argument", strlen("argument"),
                          &result_size, &ret);
@endcode

This will contact the job server, request "function" to be run with
"argument" as the argument, and return the result of that function. The
size of the result is stored in result_size. The return code is stored
in "ret" and shoulf always be checked.

@subsection client_concurrent Concurrent Client Interface

If you need to run multiple tasks at the same time, you'll want to
use the concurrent interface. After the client setup, you will then
add tasks to be run. This jsut queues the task and does not actually
run it. You then call gearman_run_tasks() to submit and process the
jobs in parallel. When using the concurrent interface, you need to
specify callback functions for each action you may care about. For
example, if you want to know when a task completes, you would pass
in a gearman_complete_fn callback function to be run for each task.

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

gearman_client_set_options(&client, GEARMAN_CLIENT_BUFFER_RESULT, 1);
gearman_client_add_task(&client, &task1, NULL, "function", "argument1",
                        strlen("argument1"), &ret);
gearman_client_add_task(&client, &task2, NULL, "function", "argument2",
                        strlen("argument2"), &ret);
gearman_client_run_tasks(&client, NULL, NULL, NULL, NULL, complete, fail);
@endcode

After adding two tasks, they are run in parallel and the complete()
callback is called when each is done (or fail() if the job failed).

@section worker Worker

Coming soon!

@section server Server

Coming soon!

*/
