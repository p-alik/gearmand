/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2008 Brian Aker, Eric Day
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <libgearman/common.h>
#include <libgearman/universal.hpp>

#include <libgearman/add.hpp>
#include <libgearman/connection.h>
#include <libgearman/packet.hpp>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <iostream>

/** Allocate a client structure.
 */
static gearman_client_st *_client_allocate(gearman_client_st *client, bool is_clone);

/**
 * Callback function used when parsing server lists.
 */
static gearman_return_t _client_add_server(const char *host, in_port_t port,
                                           void *context);

/**
 * Task state machine.
 */
static gearman_return_t _client_run_task(gearman_client_st *client,
                                         gearman_task_st *task);

/**
 * Real do function.
 */
static void *_client_do(gearman_client_st *client, gearman_command_t command,
                        const char *function_name,
                        const char *unique,
                        const void *workload, size_t workload_size,
                        size_t *result_size, gearman_return_t *ret_ptr);

/**
 * Real background do function.
 */
static gearman_return_t _client_do_background(gearman_client_st *client,
                                              gearman_command_t command,
                                              gearman_string_t &function,
                                              gearman_unique_t &unique,
                                              gearman_string_t &workload,
                                              char *job_handle);


/*
 * Public Definitions
 */

gearman_client_st *gearman_client_create(gearman_client_st *client)
{
  return _client_allocate(client, false);
}

gearman_client_st *gearman_client_clone(gearman_client_st *client,
                                        const gearman_client_st *from)
{
  if (not from)
  {
    return _client_allocate(client, false);
  }

  client= _client_allocate(client, true);

  if (client == NULL)
  {
    return client;
  }

  client->options.non_blocking= from->options.non_blocking;
  client->options.unbuffered_result= from->options.unbuffered_result;
  client->options.no_new= from->options.no_new;
  client->options.free_tasks= from->options.free_tasks;
  client->actions= from->actions;

  gearman_universal_clone(client->universal, from->universal);

  return client;
}

void gearman_client_free(gearman_client_st *client)
{
  gearman_client_task_free_all(client);

  gearman_universal_free(client->universal);

  if (client->options.allocated)
    delete client;
}

const char *gearman_client_error(const gearman_client_st *client)
{
  if (not client)
    return NULL;

  return gearman_universal_error(client->universal);
}

gearman_return_t gearman_client_error_code(const gearman_client_st *client)
{
  if (not client)
    return GEARMAN_SUCCESS;

  return gearman_universal_error_code(client->universal);
}

int gearman_client_errno(const gearman_client_st *client)
{
  if (not client)
    return 0;

  return gearman_universal_errno(client->universal);
}

gearman_client_options_t gearman_client_options(const gearman_client_st *client)
{
  int32_t options;
  memset(&options, 0, sizeof(int32_t));

  if (client->options.allocated)
    options|= int(GEARMAN_CLIENT_ALLOCATED);

  if (client->options.non_blocking)
    options|= int(GEARMAN_CLIENT_NON_BLOCKING);

  if (client->options.unbuffered_result)
    options|= int(GEARMAN_CLIENT_UNBUFFERED_RESULT);

  if (client->options.no_new)
    options|= int(GEARMAN_CLIENT_NO_NEW);

  if (client->options.free_tasks)
    options|= int(GEARMAN_CLIENT_FREE_TASKS);

  return gearman_client_options_t(options);
}

void gearman_client_set_options(gearman_client_st *client,
                                gearman_client_options_t options)
{
  gearman_client_options_t usable_options[]= {
    GEARMAN_CLIENT_NON_BLOCKING,
    GEARMAN_CLIENT_UNBUFFERED_RESULT,
    GEARMAN_CLIENT_FREE_TASKS,
    GEARMAN_CLIENT_MAX
  };

  gearman_client_options_t *ptr;


  for (ptr= usable_options; *ptr != GEARMAN_CLIENT_MAX ; ptr++)
  {
    if (options & *ptr)
    {
      gearman_client_add_options(client, *ptr);
    }
    else
    {
      gearman_client_remove_options(client, *ptr);
    }
  }
}

void gearman_client_add_options(gearman_client_st *client,
                                gearman_client_options_t options)
{
  if (options & GEARMAN_CLIENT_NON_BLOCKING)
  {
    gearman_universal_add_options(client->universal, GEARMAN_NON_BLOCKING);
    client->options.non_blocking= true;
  }

  if (options & GEARMAN_CLIENT_UNBUFFERED_RESULT)
  {
    client->options.unbuffered_result= true;
  }

  if (options & GEARMAN_CLIENT_FREE_TASKS)
  {
    client->options.free_tasks= true;
  }
}

void gearman_client_remove_options(gearman_client_st *client,
                                   gearman_client_options_t options)
{
  if (not client)
    return;

  if (options & GEARMAN_CLIENT_NON_BLOCKING)
  {
    gearman_universal_remove_options(client->universal, GEARMAN_NON_BLOCKING);
    client->options.non_blocking= false;
  }

  if (options & GEARMAN_CLIENT_UNBUFFERED_RESULT)
  {
    client->options.unbuffered_result= false;
  }

  if (options & GEARMAN_CLIENT_FREE_TASKS)
  {
    client->options.free_tasks= false;
  }
}

int gearman_client_timeout(gearman_client_st *client)
{
  return gearman_universal_timeout(client->universal);
}

void gearman_client_set_timeout(gearman_client_st *client, int timeout)
{
  if (not client)
    return;

  gearman_universal_set_timeout(client->universal, timeout);
}

void *gearman_client_context(const gearman_client_st *client)
{
  if (not client)
    return NULL;

  return const_cast<void *>(client->context);
}

void gearman_client_set_context(gearman_client_st *client, void *context)
{
  if (not client)
    return;

  client->context= context;
}

void gearman_client_set_log_fn(gearman_client_st *client,
                               gearman_log_fn *function, void *context,
                               gearman_verbose_t verbose)
{
  if (not client)
    return;

  gearman_set_log_fn(client->universal, function, context, verbose);
}

void gearman_client_set_workload_malloc_fn(gearman_client_st *client,
                                           gearman_malloc_fn *function,
                                           void *context)
{
  if (not client)
    return;

  gearman_set_workload_malloc_fn(client->universal, function, context);
}

void gearman_client_set_workload_free_fn(gearman_client_st *client, gearman_free_fn *function, void *context)
{
  if (not client)
    return;

  gearman_set_workload_free_fn(client->universal, function, context);
}

gearman_return_t gearman_client_add_server(gearman_client_st *client,
                                           const char *host, in_port_t port)
{
  if (not client)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  if (not gearman_connection_create_args(client->universal, host, port))
  {
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_client_add_servers(gearman_client_st *client,
                                            const char *servers)
{
  return gearman_parse_servers(servers, _client_add_server, client);
}

void gearman_client_remove_servers(gearman_client_st *client)
{
  if (not client)
    return;

  gearman_free_all_cons(client->universal);
}

gearman_return_t gearman_client_wait(gearman_client_st *client)
{
  return gearman_wait(client->universal);
}

void *gearman_client_do(gearman_client_st *client,
                        const char *function,
                        const char *unique,
                        const void *workload,
                        size_t workload_size, size_t *result_size,
                        gearman_return_t *ret_ptr)
{
  return _client_do(client, GEARMAN_COMMAND_SUBMIT_JOB,
                    function,
                    unique,
                    workload, workload_size,
                    result_size, ret_ptr);
}

void *gearman_client_do_high(gearman_client_st *client,
                             const char *function,
                             const char *unique,
                             const void *workload, size_t workload_size,
                             size_t *result_size, gearman_return_t *ret_ptr)
{
  return _client_do(client, GEARMAN_COMMAND_SUBMIT_JOB_HIGH,
                    function,
                    unique,
                    workload, workload_size,
                    result_size, ret_ptr);
}

void *gearman_client_do_low(gearman_client_st *client,
                            const char *function,
                            const char *unique,
                            const void *workload, size_t workload_size,
                            size_t *result_size, gearman_return_t *ret_ptr)
{
  return _client_do(client, GEARMAN_COMMAND_SUBMIT_JOB_LOW,
                    function,
                    unique,
                    workload, workload_size,
                    result_size, ret_ptr);
}

size_t gearman_client_count_tasks(gearman_client_st *client)
{
  size_t count= 1;
  gearman_task_st *search= client->task_list;

  while ((search= search->next))
  {
    count++;
  }

  return count;
}

#if 0
static bool _active_tasks(gearman_client_st *client)
{
  assert(client);
  gearman_task_st *search= client->task_list;

  if (not search)
    return false;

  do
  {
    if (gearman_task_is_active(search))
    {
      return true;
    }
  } while ((search= search->next));

  return false;
}
#endif

const char *gearman_client_do_job_handle(gearman_client_st *self)
{
  if (not self)
  {
    errno= EINVAL;
    return NULL;
  }

  if (not self->task_list)
  {
    gearman_error(self->universal, GEARMAN_INVALID_ARGUMENT, "client has an empty task list");
    return NULL;
  }

  return gearman_task_job_handle(self->task_list);
}

void gearman_client_do_status(gearman_client_st *, uint32_t *numerator, uint32_t *denominator)
{
  if (numerator)
    *numerator= 0;

  if (denominator)
    *denominator= 0;
}

gearman_return_t gearman_client_do_background(gearman_client_st *client,
                                              const char *function_name,
                                              const char *unique,
                                              const void *workload_str,
                                              size_t workload_size,
                                              char *job_handle)
{
  gearman_string_t function= { gearman_string_param_cstr(function_name) };
  gearman_unique_t local_unique= gearman_unique_make(unique, unique ? strlen(unique) : 0);
  gearman_string_t workload= { static_cast<const char*>(workload_str), workload_size };

  return _client_do_background(client, GEARMAN_COMMAND_SUBMIT_JOB_BG,
                               function,
                               local_unique,
                               workload,
                               job_handle);
}

gearman_return_t gearman_client_do_high_background(gearman_client_st *client,
                                                   const char *function_name,
                                                   const char *unique,
                                                   const void *workload_str,
                                                   size_t workload_size,
                                                   char *job_handle)
{
  gearman_string_t function= { gearman_string_param_cstr(function_name) };
  gearman_unique_t local_unique= gearman_unique_make(unique, unique ? strlen(unique) : 0);
  gearman_string_t workload= { static_cast<const char*>(workload_str), workload_size };

  return _client_do_background(client, GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG,
                               function,
                               local_unique,
                               workload,
                               job_handle);
}

gearman_return_t gearman_client_do_low_background(gearman_client_st *client,
                                                  const char *function_name,
                                                  const char *unique,
                                                  const void *workload_str,
                                                  size_t workload_size,
                                                  char *job_handle)
{
  gearman_string_t function= { gearman_string_param_cstr(function_name) };
  gearman_unique_t local_unique= gearman_unique_make(unique, unique ? strlen(unique) : 0);
  gearman_string_t workload= { static_cast<const char*>(workload_str), workload_size };

  return _client_do_background(client, GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG,
                               function,
                               local_unique,
                               workload,
                               job_handle);
}

gearman_return_t gearman_client_job_status(gearman_client_st *client,
                                           const char *job_handle,
                                           bool *is_known, bool *is_running,
                                           uint32_t *numerator,
                                           uint32_t *denominator)
{
  gearman_return_t ret;

  gearman_task_st do_task, *do_task_ptr;
  do_task_ptr= gearman_client_add_task_status(client, &do_task, client,
                                              job_handle, &ret);
  if (gearman_failed(ret))
    return ret;

  assert(do_task_ptr);

  gearman_task_clear_fn(do_task_ptr);

  ret= gearman_client_run_tasks(client);
  if (ret != GEARMAN_IO_WAIT)
  {
    if (is_known)
      *is_known= do_task.options.is_known;

    if (is_running)
      *is_running= do_task.options.is_running;

    if (numerator)
      *numerator= do_task.numerator;

    if (denominator)
      *denominator= do_task.denominator;
  }
  gearman_task_free(do_task_ptr);

  return ret;
}

gearman_return_t gearman_client_echo(gearman_client_st *client,
                                     const void *workload,
                                     size_t workload_size)
{
  if (not client)
    return GEARMAN_INVALID_ARGUMENT;

  return gearman_echo(client->universal, workload, workload_size);
}

void gearman_client_task_free_all(gearman_client_st *client)
{
  while (client->task_list)
    gearman_task_free(client->task_list);
}

void gearman_client_set_task_context_free_fn(gearman_client_st *client,
                                        gearman_task_context_free_fn *function)
{
  client->task_context_free_fn= function;
}

gearman_task_st *gearman_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         void *context,
                                         const char *function,
                                         const char *unique,
                                         const void *workload, size_t workload_size,
                                         gearman_return_t *ret_ptr)
{
  return add_task(client, task,
                  context, GEARMAN_COMMAND_SUBMIT_JOB,
                  function,
                  unique,
                  workload, workload_size,
                  time_t(0),
                  ret_ptr,
                  client->actions);
}

gearman_task_st *gearman_client_add_task_high(gearman_client_st *client,
                                              gearman_task_st *task,
                                              void *context,
                                              const char *function,
                                              const char *unique,
                                              const void *workload, size_t workload_size,
                                              gearman_return_t *ret_ptr)
{
  return add_task(client, task, context,
                  GEARMAN_COMMAND_SUBMIT_JOB_HIGH,
                  function,
                  unique,
                  workload, workload_size,
                  time_t(0),
                  ret_ptr,
                  client->actions);
}

gearman_task_st *gearman_client_add_task_low(gearman_client_st *client,
                                             gearman_task_st *task,
                                             void *context,
                                             const char *function,
                                             const char *unique,
                                             const void *workload, size_t workload_size,
                                             gearman_return_t *ret_ptr)
{
  return add_task(client, task, context, GEARMAN_COMMAND_SUBMIT_JOB_LOW,
                  function,
                  unique,
                  workload, workload_size,
                  time_t(0),
                  ret_ptr,
                  client->actions);
}

gearman_task_st *gearman_client_add_task_background(gearman_client_st *client,
                                                    gearman_task_st *task,
                                                    void *context,
                                                    const char *function,
                                                    const char *unique,
                                                    const void *workload, size_t workload_size,
                                                    gearman_return_t *ret_ptr)
{
  return add_task(client, task, context, GEARMAN_COMMAND_SUBMIT_JOB_BG,
                  function,
                  unique,
                  workload, workload_size,
                  time_t(0),
                  ret_ptr,
                  client->actions);
}

gearman_task_st *
gearman_client_add_task_high_background(gearman_client_st *client,
                                        gearman_task_st *task,
                                        void *context,
                                        const char *function,
                                        const char *unique,
                                        const void *workload, size_t workload_size,
                                        gearman_return_t *ret_ptr)
{
  return add_task(client, task, context,
                  GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG,
                  function,
                  unique,
                  workload, workload_size,
                  time_t(0),
                  ret_ptr,
                  client->actions);
}

gearman_task_st *
gearman_client_add_task_low_background(gearman_client_st *client,
                                       gearman_task_st *task,
                                       void *context,
                                       const char *function,
                                       const char *unique,
                                       const void *workload, size_t workload_size,
                                       gearman_return_t *ret_ptr)
{
  return add_task(client, task, context,
                  GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG,
                  function,
                  unique,
                  workload, workload_size,
                  time_t(0),
                  ret_ptr,
                  client->actions);

}

gearman_task_st *gearman_client_add_task_status(gearman_client_st *client,
                                                gearman_task_st *task,
                                                void *context,
                                                const char *job_handle,
                                                gearman_return_t *ret_ptr)
{
  const void *args[1];
  size_t args_size[1];
  gearman_return_t unused;

  if (not ret_ptr)
    ret_ptr= &unused;

  if (not (task= gearman_task_internal_create(client, task)))
  {
    *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  task->context= context;
  snprintf(task->job_handle, GEARMAN_JOB_HANDLE_SIZE, "%s", job_handle);

  args[0]= job_handle;
  args_size[0]= strlen(job_handle);
  gearman_return_t rc= gearman_packet_create_args(client->universal, task->send,
                                                  GEARMAN_MAGIC_REQUEST,
                                                  GEARMAN_COMMAND_GET_STATUS,
                                                  args, args_size, 1);
  if (gearman_success(rc))
  {
    client->new_tasks++;
    client->running_tasks++;
    task->options.send_in_use= true;
  }
  *ret_ptr= rc;

  return task;
}

void gearman_client_set_workload_fn(gearman_client_st *client,
                                    gearman_workload_fn *function)
{
  client->actions.workload_fn= function;
}

void gearman_client_set_created_fn(gearman_client_st *client,
                                   gearman_created_fn *function)
{
  client->actions.created_fn= function;
}

void gearman_client_set_data_fn(gearman_client_st *client,
                                gearman_data_fn *function)
{
  client->actions.data_fn= function;
}

void gearman_client_set_warning_fn(gearman_client_st *client,
                                   gearman_warning_fn *function)
{
  client->actions.warning_fn= function;
}

void gearman_client_set_status_fn(gearman_client_st *client,
                                  gearman_universal_status_fn *function)
{
  client->actions.status_fn= function;
}

void gearman_client_set_complete_fn(gearman_client_st *client,
                                    gearman_complete_fn *function)
{
  client->actions.complete_fn= function;
}

void gearman_client_set_exception_fn(gearman_client_st *client,
                                     gearman_exception_fn *function)
{
  client->actions.exception_fn= function;
}

void gearman_client_set_fail_fn(gearman_client_st *client,
                                gearman_fail_fn *function)
{
  client->actions.fail_fn= function;
}

void gearman_client_clear_fn(gearman_client_st *client)
{
  client->actions= gearman_actions_default();
}

static inline void _push_non_blocking(gearman_client_st *client)
{
  client->universal.options.stored_non_blocking= client->universal.options.non_blocking;
  client->universal.options.non_blocking= true;
}

static inline void _pop_non_blocking(gearman_client_st *client)
{
  client->universal.options.non_blocking= client->options.non_blocking;
  assert(client->universal.options.stored_non_blocking == client->options.non_blocking);
}

static inline gearman_return_t _client_run_tasks(gearman_client_st *client)
{
  gearman_return_t ret= GEARMAN_MAX_RETURN;

  switch(client->state)
  {
  case GEARMAN_CLIENT_STATE_IDLE:
    while (1)
    {
      /* Start any new tasks. */
      if (client->new_tasks > 0 && ! (client->options.no_new))
      {
        for (client->task= client->task_list; client->task;
             client->task= client->task->next)
        {
          if (client->task->state != GEARMAN_TASK_STATE_NEW)
            continue;

  case GEARMAN_CLIENT_STATE_NEW:
          gearman_return_t local_ret= _client_run_task(client, client->task);
          if (gearman_failed(local_ret) and local_ret != GEARMAN_IO_WAIT)
          {
            client->state= GEARMAN_CLIENT_STATE_NEW;

            return local_ret;
          }
        }

        if (client->new_tasks == 0)
        {
          gearman_return_t local_ret= gearman_flush_all(client->universal);
          if (gearman_failed(local_ret))
          {
            return local_ret;
          }
        }
      }

      /* See if there are any connections ready for I/O. */
      while ((client->con= gearman_ready(client->universal)))
      {
        if (client->con->revents & (POLLOUT | POLLERR | POLLHUP | POLLNVAL))
        {
          /* Socket is ready for writing, continue submitting jobs. */
          for (client->task= client->task_list; client->task;
               client->task= client->task->next)
          {
            if (client->task->con != client->con ||
                (client->task->state != GEARMAN_TASK_STATE_SUBMIT &&
                 client->task->state != GEARMAN_TASK_STATE_WORKLOAD))
            {
              continue;
            }

  case GEARMAN_CLIENT_STATE_SUBMIT:
            gearman_return_t local_ret= _client_run_task(client, client->task);
            if (gearman_failed(local_ret) and local_ret != GEARMAN_IO_WAIT)
            {
              client->state= GEARMAN_CLIENT_STATE_SUBMIT;
              return local_ret;
            }
          }
        }

        if (not (client->con->revents & POLLIN))
          continue;

        /* Socket is ready for reading. */
        while (1)
        {
          /* Read packet on connection and find which task it belongs to. */
          if (client->options.unbuffered_result)
          {
            /* If client is handling the data read, make sure it's complete. */
            if (client->con->recv_state == GEARMAN_CON_RECV_STATE_READ_DATA)
            {
              for (client->task= client->task_list; client->task;
                   client->task= client->task->next)
              {
                if (client->task->con == client->con &&
                    (client->task->state == GEARMAN_TASK_STATE_DATA ||
                     client->task->state == GEARMAN_TASK_STATE_COMPLETE))
                {
                  break;
                }
              }

              assert(client->task);
            }
            else
            {
              /* Read the next packet, without buffering the data part. */
              client->task= NULL;
              (void)client->con->recv(client->con->_packet, ret, false);
            }
          }
          else
          {
            /* Read the next packet, buffering the data part. */
            client->task= NULL;
            (void)client->con->recv(client->con->_packet, ret, true);
          }

          if (client->task == NULL)
          {
            assert(ret != GEARMAN_MAX_RETURN);

            /* Check the return of the gearman_connection_recv() calls above. */
            if (gearman_failed(ret))
            {
              if (ret == GEARMAN_IO_WAIT)
                break;

              client->state= GEARMAN_CLIENT_STATE_IDLE;
              return ret;
            }

            client->con->options.packet_in_use= true;

            /* We have a packet, see which task it belongs to. */
            for (client->task= client->task_list; client->task;
                 client->task= client->task->next)
            {
              if (client->task->con != client->con)
                continue;

              if (client->con->_packet.command == GEARMAN_COMMAND_JOB_CREATED)
              {
                if (client->task->created_id != client->con->created_id)
                  continue;

                /* New job created, drop through below and notify task. */
                client->con->created_id++;
              }
              else if (client->con->_packet.command == GEARMAN_COMMAND_ERROR)
              {
                gearman_universal_set_error(client->universal, GEARMAN_SERVER_ERROR, AT,
                                            "%s:%.*s",
                                            static_cast<char *>(client->con->_packet.arg[0]),
                                            int(client->con->_packet.arg_size[1]),
                                            static_cast<char *>(client->con->_packet.arg[1]));

                return GEARMAN_SERVER_ERROR;
              }
              else if (strncmp(client->task->job_handle,
                               static_cast<char *>(client->con->_packet.arg[0]),
                               client->con->_packet.arg_size[0]) ||
                       (client->con->_packet.command != GEARMAN_COMMAND_WORK_FAIL &&
                        strlen(client->task->job_handle) != client->con->_packet.arg_size[0] - 1) ||
                       (client->con->_packet.command == GEARMAN_COMMAND_WORK_FAIL &&
                        strlen(client->task->job_handle) != client->con->_packet.arg_size[0]))
              {
                continue;
              }

              /* Else, we have a matching result packet of some kind. */

              break;
            }

            if (not client->task)
            {
              /* The client has stopped waiting for the response, ignore it. */
              gearman_packet_free(&(client->con->_packet));
              client->con->options.packet_in_use= false;
              continue;
            }

            client->task->recv= &(client->con->_packet);
          }

  case GEARMAN_CLIENT_STATE_PACKET:
          /* Let task process job created or result packet. */
          gearman_return_t local_ret= _client_run_task(client, client->task);
          if (local_ret == GEARMAN_IO_WAIT)
            break;

          if (gearman_failed(local_ret))
          {
            client->state= GEARMAN_CLIENT_STATE_PACKET;
            return local_ret;
          }

          /* Clean up the packet. */
          gearman_packet_free(&(client->con->_packet));
          client->con->options.packet_in_use= false;

          /* If all tasks are done, return. */
          if (client->running_tasks == 0)
            break;
        }
      }

      /* If all tasks are done, return. */
      if (client->running_tasks == 0)
      {
        break;
      }

      if (client->new_tasks > 0 && ! (client->options.no_new))
        continue;

      if (client->options.non_blocking)
      {
        /* Let the caller wait for activity. */
        client->state= GEARMAN_CLIENT_STATE_IDLE;
        gearman_gerror(client->universal, GEARMAN_IO_WAIT);

        return GEARMAN_IO_WAIT;
      }

      /* Wait for activity on one of the connections. */
      gearman_return_t local_ret= gearman_wait(client->universal);
      if (gearman_failed(local_ret) and local_ret != GEARMAN_IO_WAIT)
      {
        client->state= GEARMAN_CLIENT_STATE_IDLE;

        return local_ret;
      }
    }

    break;
  }

  client->state= GEARMAN_CLIENT_STATE_IDLE;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_client_run_tasks(gearman_client_st *client)
{
  if (not client)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  if (not client->task_list)
  {
    return gearman_error(client->universal, GEARMAN_INVALID_ARGUMENT, "No active tasks");
  }


  _push_non_blocking(client);

  gearman_return_t rc= _client_run_tasks(client);

  _pop_non_blocking(client);

  std::cerr << std::endl << gearman_strerror(rc) << " " << __func__ << std::endl;
  if (gearman_failed(rc))
  {
    gearman_gerror(client->universal, rc);
#if 0
    if (rc != gearman_universal_error_code(client->universal))
    {
      std::cerr << "print error bad, expected " << gearman_strerror(rc) << " and got " << gearman_strerror(gearman_universal_error_code(client->universal)) << std::endl;
      std::cerr << "\t" << gearman_client_error(client) << " " << &client->universal << std::endl;
    }
#endif
    assert(gearman_universal_error_code(client->universal) == rc);
  }

  return rc;
}

/*
 * Static Definitions
 */

static gearman_client_st *_client_allocate(gearman_client_st *client, bool is_clone)
{
  if (client)
  {
    client->options.allocated= false;
  }
  else
  {
    client= new (std::nothrow) gearman_client_st;
    if (not client)
      return NULL;

    client->options.allocated= true;
  }

  client->options.non_blocking= false;
  client->options.unbuffered_result= false;
  client->options.no_new= false;
  client->options.free_tasks= false;

  client->state= GEARMAN_CLIENT_STATE_IDLE;
  client->new_tasks= 0;
  client->running_tasks= 0;
  client->task_count= 0;
  client->context= NULL;
  client->con= NULL;
  client->task= NULL;
  client->task_list= NULL;
  client->task_context_free_fn= NULL;
  gearman_client_clear_fn(client);

  if (not is_clone)
  {
    gearman_universal_initialize(client->universal);
  }

  return client;
}

static gearman_return_t _client_add_server(const char *host, in_port_t port,
                                           void *context)
{
  return gearman_client_add_server(static_cast<gearman_client_st *>(context), host, port);
}

static gearman_return_t _client_run_task(gearman_client_st *client, gearman_task_st *task)
{
  switch(task->state)
  {
  case GEARMAN_TASK_STATE_NEW:
    if (task->client->universal.con_list == NULL)
    {
      client->new_tasks--;
      client->running_tasks--;
      gearman_universal_set_error(client->universal, GEARMAN_NO_SERVERS, __func__, AT, "no servers added");
      return GEARMAN_NO_SERVERS;
    }

    for (task->con= task->client->universal.con_list; task->con;
         task->con= task->con->next)
    {
      if (task->con->send_state == GEARMAN_CON_SEND_STATE_NONE)
        break;
    }

    if (task->con == NULL)
    {
      client->options.no_new= true;
      gearman_gerror(client->universal, GEARMAN_IO_WAIT);
      return GEARMAN_IO_WAIT;
    }

    client->new_tasks--;

    if (task->send.command != GEARMAN_COMMAND_GET_STATUS)
    {
      task->created_id= task->con->created_id_next;
      task->con->created_id_next++;
    }

  case GEARMAN_TASK_STATE_SUBMIT:
    while (1)
    {
      gearman_return_t ret= task->con->send(task->send, client->new_tasks == 0 ? true : false);

      if (gearman_success(ret))
      {
        break;
      }
      else if (ret == GEARMAN_IO_WAIT)
      {
        task->state= GEARMAN_TASK_STATE_SUBMIT;
        return ret;
      }
      else if (gearman_failed(ret))
      {
        /* Increment this since the job submission failed. */
        task->con->created_id++;

        if (ret == GEARMAN_COULD_NOT_CONNECT)
        {
          for (task->con= task->con->next; task->con;
               task->con= task->con->next)
          {
            if (task->con->send_state == GEARMAN_CON_SEND_STATE_NONE)
              break;
          }
        }
        else
        {
          task->con= NULL;
        }

        if (task->con == NULL)
        {
          client->running_tasks--;
          return ret;
        }

        if (task->send.command != GEARMAN_COMMAND_GET_STATUS)
        {
          task->created_id= task->con->created_id_next;
          task->con->created_id_next++;
        }
      }
    }

    if (task->send.data_size > 0 && task->send.data == NULL)
    {
      if (not task->func.workload_fn)
      {
        gearman_error(client->universal, GEARMAN_NEED_WORKLOAD_FN,
                      "workload size > 0, but no data pointer or workload_fn was given");
        return GEARMAN_NEED_WORKLOAD_FN;
      }

  case GEARMAN_TASK_STATE_WORKLOAD:
      gearman_return_t ret= task->func.workload_fn(task);
      if (gearman_failed(ret))
      {
        task->state= GEARMAN_TASK_STATE_WORKLOAD;
        return ret;
      }
    }

    client->options.no_new= false;
    task->state= GEARMAN_TASK_STATE_WORK;
    task->con->set_events(POLLIN);
    return GEARMAN_SUCCESS;

  case GEARMAN_TASK_STATE_WORK:
    if (task->recv->command == GEARMAN_COMMAND_JOB_CREATED)
    {
      snprintf(task->job_handle, GEARMAN_JOB_HANDLE_SIZE, "%.*s",
               int(task->recv->arg_size[0]),
               static_cast<char *>(task->recv->arg[0]));

  case GEARMAN_TASK_STATE_CREATED:
      if (task->func.created_fn)
      {
        gearman_return_t ret= task->func.created_fn(task);
        if (gearman_failed(ret))
        {
          task->state= GEARMAN_TASK_STATE_CREATED;
          return ret;
        }
      }

      if (task->send.command == GEARMAN_COMMAND_SUBMIT_JOB_BG ||
          task->send.command == GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG ||
          task->send.command == GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG ||
          task->send.command == GEARMAN_COMMAND_SUBMIT_JOB_EPOCH ||
          task->send.command == GEARMAN_COMMAND_SUBMIT_REDUCE_JOB_BACKGROUND)
      {
        break;
      }
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_DATA)
    {
  case GEARMAN_TASK_STATE_DATA:
      if (task->func.data_fn)
      {
        gearman_return_t ret= task->func.data_fn(task);
        if (gearman_failed(ret))
        {
          task->state= GEARMAN_TASK_STATE_DATA;
          return ret;
        }
      }
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_WARNING)
    {
  case GEARMAN_TASK_STATE_WARNING:
      if (task->func.warning_fn)
      {
        gearman_return_t ret= task->func.warning_fn(task);
        if (gearman_failed(ret))
        {
          task->state= GEARMAN_TASK_STATE_WARNING;
          return ret;
        }
      }
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_STATUS ||
             task->recv->command == GEARMAN_COMMAND_STATUS_RES)
    {
      uint8_t x;

      if (task->recv->command == GEARMAN_COMMAND_STATUS_RES)
      {
        if (atoi(static_cast<char *>(task->recv->arg[1])) == 0)
          task->options.is_known= false;
        else
          task->options.is_known= true;

        if (atoi(static_cast<char *>(task->recv->arg[2])) == 0)
          task->options.is_running= false;
        else
          task->options.is_running= true;

        x= 3;
      }
      else
      {
        x= 1;
      }

      task->numerator= uint32_t(atoi(static_cast<char *>(task->recv->arg[x])));
      char status_buffer[11]; /* Max string size to hold a uint32_t. */
      snprintf(status_buffer, 11, "%.*s",
               int(task->recv->arg_size[x + 1]),
               static_cast<char *>(task->recv->arg[x + 1]));
      task->denominator= uint32_t(atoi(status_buffer));

  case GEARMAN_TASK_STATE_STATUS:
      if (task->func.status_fn)
      {
        gearman_return_t ret= task->func.status_fn(task);
        if (gearman_failed(ret))
        {
          task->state= GEARMAN_TASK_STATE_STATUS;
          return ret;
        }
      }

      if (task->send.command == GEARMAN_COMMAND_GET_STATUS)
        break;
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_COMPLETE)
    {
      task->result_rc= GEARMAN_SUCCESS;

  case GEARMAN_TASK_STATE_COMPLETE:
      if (task->func.complete_fn)
      {
        gearman_return_t ret= task->func.complete_fn(task);
        if (gearman_failed(ret))
        {
          task->state= GEARMAN_TASK_STATE_COMPLETE;
          return ret;
        }
      }

      break;
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_EXCEPTION)
    {
  case GEARMAN_TASK_STATE_EXCEPTION:
      if (task->func.exception_fn)
      {
        gearman_return_t ret= task->func.exception_fn(task);
        if (gearman_failed(ret))
        {
          task->state= GEARMAN_TASK_STATE_EXCEPTION;
          return ret;
        }
      }
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_FAIL)
    {
      // If things fail we need to delete the result, and set the result_rc
      // correctly.
      delete task->result_ptr;
      task->result_ptr= NULL;
      task->result_rc= GEARMAN_WORK_FAIL;

  case GEARMAN_TASK_STATE_FAIL:
      if (task->func.fail_fn)
      {
        gearman_return_t ret= task->func.fail_fn(task);
        if (gearman_failed(ret))
        {
          task->state= GEARMAN_TASK_STATE_FAIL;
          return ret;
        }
      }

      break;
    }

    task->state= GEARMAN_TASK_STATE_WORK;
    return GEARMAN_SUCCESS;

  case GEARMAN_TASK_STATE_FINISHED:
    break;
  }

  client->running_tasks--;
  task->state= GEARMAN_TASK_STATE_FINISHED;

  if (client->options.free_tasks)
  {
    gearman_task_free(task);
  }

  return GEARMAN_SUCCESS;
}

static void *_client_do(gearman_client_st *client, gearman_command_t command,
                        const char *function_name,
                        const char *unique,
                        const void *workload_str, size_t workload_size,
                        size_t *result_size, gearman_return_t *ret_ptr)
{
  gearman_task_st do_task, *do_task_ptr;
  gearman_client_task_free_all(client);
  gearman_string_t function= { gearman_string_param_cstr(function_name) };
  gearman_unique_t local_unique= gearman_unique_make(unique, unique ? strlen(unique) : 0);
  gearman_string_t workload= { static_cast<const char*>(workload_str), workload_size };

  gearman_return_t unused;
  if (not ret_ptr)
    ret_ptr= &unused;

  if (not client)
  {
    *ret_ptr= GEARMAN_ERRNO;
    errno= EINVAL;
    return NULL;
  }

  do_task_ptr= add_task(client, &do_task, NULL, command,
                        function,
                        local_unique,
                        workload,
                        time_t(0),
                        gearman_actions_do_default());
  if (not do_task_ptr)
  {
    *ret_ptr= gearman_universal_error_code(client->universal);
    return NULL;
  }

  gearman_return_t ret= gearman_client_run_tasks(client);

  const void *returnable= NULL;
  // gearman_client_run_tasks failed
  if (gearman_failed(ret))
  {
    gearman_error(client->universal, ret, "occured during gearman_client_run_tasks()");

    *ret_ptr= ret;
    *result_size= 0;
  }
  else if (ret == GEARMAN_SUCCESS and do_task_ptr->result_rc == GEARMAN_SUCCESS)
  {
    *ret_ptr= do_task_ptr->result_rc;
    assert(do_task_ptr);
    if (do_task_ptr->result_ptr)
    {
      gearman_string_t result= gearman_result_take_string(do_task_ptr->result_ptr);
      *result_size= gearman_size(result);
      returnable= gearman_c_str(result);
    }
    else // NULL job
    {
      *result_size= 0;
    }
  }
  else // gearman_client_run_tasks() was successful, but the task was not
  {
    gearman_error(client->universal, do_task_ptr->result_rc, "occured during gearman_client_run_tasks()");

    *ret_ptr= do_task_ptr->result_rc;
    *result_size= 0;
  }

  assert(client->task_list);
  gearman_task_free(&do_task);
  client->new_tasks= 0;
  client->running_tasks= 0;

  return const_cast<void *>(returnable);
}

static gearman_return_t _client_do_background(gearman_client_st *client,
                                              gearman_command_t command,
                                              gearman_string_t &function,
                                              gearman_unique_t &unique,
                                              gearman_string_t &workload,
                                              char *job_handle)
{
  gearman_task_st do_task, *do_task_ptr;
  do_task_ptr= add_task(client, &do_task, 
                        client, 
                        command,
                        function,
                        unique,
                        workload,
                        time_t(0),
                        gearman_actions_do_default());
  if (not do_task_ptr)
  {
    return gearman_universal_error_code(client->universal);
  }

  gearman_task_clear_fn(do_task_ptr);

  gearman_return_t ret= gearman_client_run_tasks(client);
  if (ret != GEARMAN_IO_WAIT)
  {
    if (job_handle)
    {
      strncpy(job_handle, do_task.job_handle, GEARMAN_JOB_HANDLE_SIZE);
    }
    client->new_tasks= 0;
    client->running_tasks= 0;
  }
  gearman_task_free(&do_task);

  return ret;
}

bool gearman_client_compare(const gearman_client_st *first, const gearman_client_st *second)
{
  if (not first || not second)
    return false;

  if (strcmp(first->universal.con_list->host, second->universal.con_list->host))
    return false;

  if (first->universal.con_list->port != second->universal.con_list->port)
    return false;

  return true;
}

bool gearman_client_set_server_option(gearman_client_st *self, const char *option_arg, size_t option_arg_size)
{
  gearman_string_t option= { option_arg, option_arg_size };

  return gearman_request_option(self->universal, option);
}

void gearman_client_set_namespace(gearman_client_st *self, const char *namespace_key, size_t namespace_key_size)
{
  if (not self)
    return;

  gearman_universal_set_namespace(self->universal, namespace_key, namespace_key_size);
}
