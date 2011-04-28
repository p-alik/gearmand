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

/**
 * @file
 * @brief Client Definitions
 */

#include <libgearman/common.h>

#include <libgearman/add.h>
#include <libgearman/connection.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

/**
 * @addtogroup gearman_client_static Static Client Declarations
 * @ingroup gearman_client
 * @{
 */

/**
 * Allocate a client structure.
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
                        const char *function_name, size_t functiona_name_length,
                        const char *unique, size_t unique_length,
                        const void *workload, size_t workload_size,
                        size_t *result_size, gearman_return_t *ret_ptr);

/**
 * Real background do function.
 */
static gearman_return_t _client_do_background(gearman_client_st *client,
                                              gearman_command_t command,
                                              const char *function_name,
                                              size_t functiona_name_length,
                                              const char *unique,
                                              size_t unique_length,
                                              const void *workload,
                                              size_t workload_size,
                                              char *job_handle);

/** @} */

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
  gearman_universal_st *check;

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

  check= gearman_universal_clone((&client->universal), &(from->universal));
  if (not check)
  {
    gearman_client_free(client);
    return NULL;
  }

  return client;
}

void gearman_client_free(gearman_client_st *client)
{
  gearman_client_task_free_all(client);

  gearman_universal_free(&client->universal);

  if (client->options.allocated)
    delete client;
}

const char *gearman_client_error(const gearman_client_st *client)
{
  if (not client)
    return NULL;

  return gearman_universal_error(&client->universal);
}

gearman_return_t gearman_client_error_code(const gearman_client_st *client)
{
  return gearman_universal_error_code(&client->universal);
}

int gearman_client_errno(const gearman_client_st *client)
{
  return gearman_universal_errno(&client->universal);
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
    gearman_universal_add_options(&client->universal, GEARMAN_NON_BLOCKING);
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
  if (options & GEARMAN_CLIENT_NON_BLOCKING)
  {
    gearman_universal_remove_options(&client->universal, GEARMAN_NON_BLOCKING);
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
  return gearman_universal_timeout(&client->universal);
}

void gearman_client_set_timeout(gearman_client_st *client, int timeout)
{
  gearman_universal_set_timeout(&client->universal, timeout);
}

void *gearman_client_context(const gearman_client_st *client)
{
  return const_cast<void *>(client->context);
}

void gearman_client_set_context(gearman_client_st *client, void *context)
{
  client->context= context;
}

void gearman_client_set_log_fn(gearman_client_st *client,
                               gearman_log_fn *function, void *context,
                               gearman_verbose_t verbose)
{
  gearman_set_log_fn(&client->universal, function, context, verbose);
}

void gearman_client_set_workload_malloc_fn(gearman_client_st *client,
                                           gearman_malloc_fn *function,
                                           void *context)
{
  gearman_set_workload_malloc_fn(&client->universal, function, context);
}

void gearman_client_set_workload_free_fn(gearman_client_st *client, gearman_free_fn *function, void *context)
{
  gearman_set_workload_free_fn(&client->universal, function, context);
}

gearman_return_t gearman_client_add_server(gearman_client_st *client,
                                           const char *host, in_port_t port)
{
  if (not client)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  if (gearman_connection_create_args(&client->universal, NULL, host, port) == NULL)
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
  gearman_free_all_cons(&client->universal);
}

gearman_return_t gearman_client_wait(gearman_client_st *client)
{
  return gearman_wait(&client->universal);
}

void *gearman_client_do(gearman_client_st *client, const char *function_name,
                        const char *unique, const void *workload,
                        size_t workload_size, size_t *result_size,
                        gearman_return_t *ret_ptr)
{
  return _client_do(client, GEARMAN_COMMAND_SUBMIT_JOB,
                    function_name, strlen(function_name),
                    unique, unique ? strlen(unique) : 0,
                    workload, workload_size, result_size, ret_ptr);
}

void *gearman_client_do_high(gearman_client_st *client,
                             const char *function_name, const char *unique,
                             const void *workload, size_t workload_size,
                             size_t *result_size, gearman_return_t *ret_ptr)
{
  return _client_do(client, GEARMAN_COMMAND_SUBMIT_JOB_HIGH,
                    function_name, strlen(function_name),
                    unique, unique ? strlen(unique) : 0,
                    workload, workload_size, result_size, ret_ptr);
}

void *gearman_client_do_low(gearman_client_st *client,
                            const char *function_name, const char *unique,
                            const void *workload, size_t workload_size,
                            size_t *result_size, gearman_return_t *ret_ptr)
{
  return _client_do(client, GEARMAN_COMMAND_SUBMIT_JOB_LOW,
                    function_name, strlen(function_name),
                    unique, unique ? strlen(unique) : 0,
                    workload, workload_size, result_size, ret_ptr);
}

static inline gearman_command_t pick_command_by_priority(const gearman_job_priority_t &arg)
{
  if (arg == GEARMAN_JOB_PRIORITY_NORMAL)
    return GEARMAN_COMMAND_SUBMIT_JOB;
  else if (arg == GEARMAN_JOB_PRIORITY_HIGH)
    return GEARMAN_COMMAND_SUBMIT_JOB_HIGH;

  return GEARMAN_COMMAND_SUBMIT_JOB_LOW;
}

static inline gearman_command_t pick_command_by_priority_background(const gearman_job_priority_t &arg)
{
  if (arg == GEARMAN_JOB_PRIORITY_NORMAL)
    return GEARMAN_COMMAND_SUBMIT_JOB_BG;
  else if (arg == GEARMAN_JOB_PRIORITY_HIGH)
    return GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG;

  return GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG;
}

#if 0
static size_t _count_tasks(gearman_client_st *client)
{
  size_t count= 1;
  gearman_task_st *search= client->task_list;

  while ((search= search->next))
  {
    count++;
  }

  return count;
}
#endif

static gearman_return_t _client_execute_setup(gearman_client_st *client,
                                              const gearman_function_st *function,
                                              gearman_workload_t *workload,
                                              bool &has_background)
{
  gearman_workload_t *workload_ptr= workload;

  gearman_command_t command;
  if (gearman_workload_epoch(workload_ptr))
  {
    command= GEARMAN_COMMAND_SUBMIT_JOB_EPOCH;
  }
  else if (gearman_workload_background(workload_ptr))
  {
    command= pick_command_by_priority_background(gearman_workload_priority(workload_ptr));
  }
  else
  {
    command= pick_command_by_priority(gearman_workload_priority(workload_ptr));
  }

  gearman_task_st *task= add_task(client, NULL, 
                                  gearman_workload_context(workload_ptr),
                                  command,
                                          gearman_function_name(function), gearman_function_size(function),
                                          gearman_param(&workload_ptr->unique),
                                          gearman_param(workload_ptr),
                                          gearman_workload_epoch(workload_ptr));

  gearman_workload_set_task(workload_ptr, task);

  if (not task)
  {
    gearman_client_task_free_all(client);
    return gearman_universal_error_code(&client->universal);
  }

  if (gearman_workload_background(workload_ptr))
  {
    has_background= true;
  }
  else // Define do operations
  {
    task->func= gearman_actions_do_default();
  }

  return GEARMAN_SUCCESS;
}

static bool _client_execute(gearman_client_st *client,
                                        bool &has_background)
{
  if (not has_background)
  {
    gearman_return_t rc;

    do {
      rc= gearman_client_run_tasks(client);
    } while (rc == GEARMAN_PAUSE);

    if (rc == GEARMAN_SUCCESS)
    { }
    else
    {
      gearman_error(&client->universal, rc, "failure occured during gearman_client_run_tasks()");
      return false;
    }
  }
  else
  {
    gearman_return_t rc= gearman_client_run_tasks(client);
    gearman_error(&client->universal, rc, "gearman_client_run_tasks()");

    if (rc != GEARMAN_IO_WAIT)
    {
      client->new_tasks= 0;
      client->running_tasks= 0;
    }
  }

  return true;
}

bool gearman_client_execute(gearman_client_st *client,
                            const gearman_function_st *function,
                            gearman_workload_t *workload)
{
  if (not client)
  {
    errno= EINVAL;
    return GEARMAN_ERRNO;
  }

  if (not function)
  {
    errno= EINVAL;
    gearman_perror(&client->universal, "gearman_function_st was NULL");
    return GEARMAN_ERRNO;
  }

  if (not workload)
  {
    errno= EINVAL;
    gearman_perror((&client->universal), "gearman_workload_t was NULL");
    return GEARMAN_ERRNO;
  }

  gearman_workload_t *workload_ptr= workload;
  bool has_background= false;
  for (uint32_t x= 0; x < gearman_workload_array_size(*workload_ptr); x++, workload_ptr++)
  {
    _client_execute_setup(client, function, workload, has_background);
  }

  return _client_execute(client, has_background);
}


bool gearman_client_execute_batch(gearman_client_st *client,
                                  gearman_batch_t *batch)
{
  if (not client)
  {
    errno= EINVAL;
    return false;
  }

  if (not batch)
  {
    errno= EINVAL;
    gearman_perror(&client->universal, "gearman_workload_batch_st was NULL");
    return false;
  }

  bool has_background= false;
  gearman_batch_t *workload_ptr= batch;
  for (uint32_t x= 0; x < gearman_workload_batch_array_size(*workload_ptr); x++, workload_ptr++)
  {
    _client_execute_setup(client, workload_ptr->function, &workload_ptr->workload, has_background);
  }

  return _client_execute(client, has_background);
}

const char *gearman_client_do_job_handle(const gearman_client_st *self)
{
  if (not self)
  {
    errno= EINVAL;
    return NULL;
  }

  if (not self->task_list)
  {
    errno= EINVAL;
    return NULL;
  }

  return gearman_task_job_handle(self->task_list);
}

void gearman_client_do_status(gearman_client_st *, uint32_t *numerator, uint32_t *denominator)
{
  if (numerator != NULL)
    *numerator= 0;

  if (denominator != NULL)
    *denominator= 0;
}

gearman_return_t gearman_client_do_background(gearman_client_st *client,
                                              const char *function_name,
                                              const char *unique,
                                              const void *workload,
                                              size_t workload_size,
                                              char *job_handle)
{
  return _client_do_background(client, GEARMAN_COMMAND_SUBMIT_JOB_BG,
                               function_name, strlen(function_name),
                               unique, unique ? strlen(unique) : 0,
                               workload, workload_size,
                               job_handle);
}

gearman_return_t gearman_client_do_high_background(gearman_client_st *client,
                                                   const char *function_name,
                                                   const char *unique,
                                                   const void *workload,
                                                   size_t workload_size,
                                                   char *job_handle)
{
  return _client_do_background(client, GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG,
                               function_name, strlen(function_name),
                               unique, unique ? strlen(unique) : 0,
                               workload, workload_size,
                               job_handle);
}

gearman_return_t gearman_client_do_low_background(gearman_client_st *client,
                                                  const char *function_name,
                                                  const char *unique,
                                                  const void *workload,
                                                  size_t workload_size,
                                                  char *job_handle)
{
  return _client_do_background(client, GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG,
                               function_name, strlen(function_name),
                               unique, unique ? strlen(unique) : 0,
                               workload, workload_size,
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
  if (ret != GEARMAN_SUCCESS)
    return ret;

  assert(do_task_ptr);

  gearman_task_clear_fn(do_task_ptr);

  ret= gearman_client_run_tasks(client);
  if (ret != GEARMAN_IO_WAIT)
  {
    if (is_known != NULL)
      *is_known= do_task.options.is_known;
    if (is_running != NULL)
      *is_running= do_task.options.is_running;
    if (numerator != NULL)
      *numerator= do_task.numerator;
    if (denominator != NULL)
      *denominator= do_task.denominator;
  }
  gearman_task_free(do_task_ptr);

  return ret;
}

gearman_return_t gearman_client_echo(gearman_client_st *client,
                                     const void *workload,
                                     size_t workload_size)
{
  return gearman_echo(&client->universal, workload, workload_size);
}

void gearman_client_task_free_all(gearman_client_st *client)
{
  while (client->task_list != NULL)
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
                                         const char *function_name,
                                         const char *unique,
                                         const void *workload,
                                         size_t workload_size,
                                         gearman_return_t *ret_ptr)
{
  task= add_task(client, task, context, GEARMAN_COMMAND_SUBMIT_JOB,
                 function_name, strlen(function_name),
                 unique, unique ? strlen(unique) : 0,
                 workload, workload_size,
                 0);

  if (not task and ret_ptr)
  {
    *ret_ptr= gearman_universal_error_code(&client->universal);
  }
  else if (ret_ptr)
  {
    *ret_ptr= GEARMAN_SUCCESS;
  }

  return task;
}

gearman_task_st *gearman_client_add_task_high(gearman_client_st *client,
                                              gearman_task_st *task,
                                              void *context,
                                              const char *function_name,
                                              const char *unique,
                                              const void *workload,
                                              size_t workload_size,
                                              gearman_return_t *ret_ptr)
{
  task= add_task(client, task, context,
                 GEARMAN_COMMAND_SUBMIT_JOB_HIGH,
                 function_name, strlen(function_name),
                 unique, unique ? strlen(unique) : 0,
                 workload, workload_size, 0);

  if (not task and ret_ptr)
  {
    *ret_ptr= gearman_universal_error_code(&client->universal);
  }
  else if (ret_ptr)
  {
    *ret_ptr= GEARMAN_SUCCESS;
  }

  return task;
}

gearman_task_st *gearman_client_add_task_low(gearman_client_st *client,
                                             gearman_task_st *task,
                                             void *context,
                                             const char *function_name,
                                             const char *unique,
                                             const void *workload,
                                             size_t workload_size,
                                             gearman_return_t *ret_ptr)
{
  task= add_task(client, task, context, GEARMAN_COMMAND_SUBMIT_JOB_LOW,
                 function_name, strlen(function_name),
                 unique, unique ? strlen(unique) : 0,
                 workload, workload_size,
                 0);

  if (not task and ret_ptr)
  {
    *ret_ptr= gearman_universal_error_code(&client->universal);
  }
  else if (ret_ptr)
  {
    *ret_ptr= GEARMAN_SUCCESS;
  }

  return task;
}

gearman_task_st *gearman_client_add_task_background(gearman_client_st *client,
                                                    gearman_task_st *task,
                                                    void *context,
                                                    const char *function_name,
                                                    const char *unique,
                                                    const void *workload,
                                                    size_t workload_size,
                                                    gearman_return_t *ret_ptr)
{
  task= add_task(client, task, context, GEARMAN_COMMAND_SUBMIT_JOB_BG,
                 function_name, strlen(function_name),
                 unique, unique ? strlen(unique) : 0,
                 workload, workload_size,
                 0);

  if (not task and ret_ptr)
  {
    *ret_ptr= gearman_universal_error_code(&client->universal);
  }
  else if (ret_ptr)
  {
    *ret_ptr= GEARMAN_SUCCESS;
  }

  return task;
}

gearman_task_st *
gearman_client_add_task_high_background(gearman_client_st *client,
                                        gearman_task_st *task,
                                        void *context,
                                        const char *function_name,
                                        const char *unique,
                                        const void *workload,
                                        size_t workload_size,
                                        gearman_return_t *ret_ptr)
{
  task= add_task(client, task, context,
                 GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG,
                 function_name, strlen(function_name),
                 unique, unique ? strlen(unique) : 0,
                 workload, workload_size,
                 0);

  if (not task and ret_ptr)
  {
    *ret_ptr= gearman_universal_error_code(&client->universal);
  }
  else if (ret_ptr)
  {
    *ret_ptr= GEARMAN_SUCCESS;
  }
  return task;
}

gearman_task_st *
gearman_client_add_task_low_background(gearman_client_st *client,
                                       gearman_task_st *task,
                                       void *context,
                                       const char *function_name,
                                       const char *unique,
                                       const void *workload,
                                       size_t workload_size,
                                       gearman_return_t *ret_ptr)
{
  task= add_task(client, task, context,
                 GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG,
                 function_name, strlen(function_name),
                 unique, unique ? strlen(unique) : 0,
                 workload, workload_size, 
                 0);

  if (not task and ret_ptr)
  {
    *ret_ptr= gearman_universal_error_code(&client->universal);
  }
  else if (ret_ptr)
  {
    *ret_ptr= GEARMAN_SUCCESS;
  }

  return task;
}

gearman_task_st *gearman_client_add_task_status(gearman_client_st *client,
                                                gearman_task_st *task,
                                                void *context,
                                                const char *job_handle,
                                                gearman_return_t *ret_ptr)
{
  const void *args[1];
  size_t args_size[1];

  task= gearman_task_create(client, task);
  if (task == NULL)
  {
    if (ret_ptr)
      *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  task->context= context;
  snprintf(task->job_handle, GEARMAN_JOB_HANDLE_SIZE, "%s", job_handle);

  args[0]= job_handle;
  args_size[0]= strlen(job_handle);
  gearman_return_t rc;
  rc= gearman_packet_create_args(&client->universal, &(task->send),
                                 GEARMAN_MAGIC_REQUEST,
                                 GEARMAN_COMMAND_GET_STATUS,
                                 args, args_size, 1);
  if (rc == GEARMAN_SUCCESS)
  {
    client->new_tasks++;
    client->running_tasks++;
    task->options.send_in_use= true;
  }

  if (ret_ptr)
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
  gearman_return_t ret;

  switch(client->state)
  {
  case GEARMAN_CLIENT_STATE_IDLE:
    while (1)
    {
      /* Start any new tasks. */
      if (client->new_tasks > 0 && ! (client->options.no_new))
      {
        for (client->task= client->task_list; client->task != NULL;
             client->task= client->task->next)
        {
          if (client->task->state != GEARMAN_TASK_STATE_NEW)
            continue;

  case GEARMAN_CLIENT_STATE_NEW:
          ret= _client_run_task(client, client->task);
          if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
          {
            client->state= GEARMAN_CLIENT_STATE_NEW;

            return ret;
          }
        }

        if (client->new_tasks == 0)
        {
          ret= gearman_flush_all(&client->universal);
          if (ret != GEARMAN_SUCCESS)
          {
            return ret;
          }
        }
      }

      /* See if there are any connections ready for I/O. */
      while ((client->con= gearman_ready(&client->universal)) != NULL)
      {
        if (client->con->revents & (POLLOUT | POLLERR | POLLHUP | POLLNVAL))
        {
          /* Socket is ready for writing, continue submitting jobs. */
          for (client->task= client->task_list; client->task != NULL;
               client->task= client->task->next)
          {
            if (client->task->con != client->con ||
                (client->task->state != GEARMAN_TASK_STATE_SUBMIT &&
                 client->task->state != GEARMAN_TASK_STATE_WORKLOAD))
            {
              continue;
            }

  case GEARMAN_CLIENT_STATE_SUBMIT:
            ret= _client_run_task(client, client->task);
            if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
            {
              client->state= GEARMAN_CLIENT_STATE_SUBMIT;
              return ret;
            }
          }
        }

        if (! (client->con->revents & POLLIN))
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
              for (client->task= client->task_list; client->task != NULL;
                   client->task= client->task->next)
              {
                if (client->task->con == client->con &&
                    (client->task->state == GEARMAN_TASK_STATE_DATA ||
                     client->task->state == GEARMAN_TASK_STATE_COMPLETE))
                {
                  break;
                }
              }

              assert(client->task != NULL);
            }
            else
            {
              /* Read the next packet, without buffering the data part. */
              client->task= NULL;
              (void)gearman_connection_recv(client->con, &(client->con->packet), &ret,
                                     false);
            }
          }
          else
          {
            /* Read the next packet, buffering the data part. */
            client->task= NULL;
            (void)gearman_connection_recv(client->con, &(client->con->packet), &ret, true);
          }

          if (client->task == NULL)
          {
            /* Check the return of the gearman_connection_recv() calls above. */
            if (ret != GEARMAN_SUCCESS)
            {
              if (ret == GEARMAN_IO_WAIT)
                break;

              client->state= GEARMAN_CLIENT_STATE_IDLE;
              return ret;
            }

            client->con->options.packet_in_use= true;

            /* We have a packet, see which task it belongs to. */
            for (client->task= client->task_list; client->task != NULL;
                 client->task= client->task->next)
            {
              if (client->task->con != client->con)
                continue;

              if (client->con->packet.command == GEARMAN_COMMAND_JOB_CREATED)
              {
                if (client->task->created_id != client->con->created_id)
                  continue;

                /* New job created, drop through below and notify task. */
                client->con->created_id++;
              }
              else if (client->con->packet.command == GEARMAN_COMMAND_ERROR)
              {
                gearman_universal_set_error(&client->universal,
                                            GEARMAN_SERVER_ERROR,
                                            "gearman_client_run_tasks",
                                            "%s:%.*s",
                                            static_cast<char *>(client->con->packet.arg[0]),
                                            int(client->con->packet.arg_size[1]),
                                            static_cast<char *>(client->con->packet.arg[1]));

                return GEARMAN_SERVER_ERROR;
              }
              else if (strncmp(client->task->job_handle,
                               static_cast<char *>(client->con->packet.arg[0]),
                               client->con->packet.arg_size[0]) ||
                       (client->con->packet.command != GEARMAN_COMMAND_WORK_FAIL &&
                        strlen(client->task->job_handle) != client->con->packet.arg_size[0] - 1) ||
                       (client->con->packet.command == GEARMAN_COMMAND_WORK_FAIL &&
                        strlen(client->task->job_handle) != client->con->packet.arg_size[0]))
              {
                continue;
              }

              /* Else, we have a matching result packet of some kind. */

              break;
            }

            if (client->task == NULL)
            {
              /* The client has stopped waiting for the response, ignore it. */
              gearman_packet_free(&(client->con->packet));
              client->con->options.packet_in_use= false;
              continue;
            }

            client->task->recv= &(client->con->packet);
          }

  case GEARMAN_CLIENT_STATE_PACKET:
          /* Let task process job created or result packet. */
          ret= _client_run_task(client, client->task);
          if (ret == GEARMAN_IO_WAIT)
            break;
          if (ret != GEARMAN_SUCCESS)
          {
            client->state= GEARMAN_CLIENT_STATE_PACKET;
            return ret;
          }

          /* Clean up the packet. */
          gearman_packet_free(&(client->con->packet));
          client->con->options.packet_in_use= false;

          /* If all tasks are done, return. */
          if (client->running_tasks == 0)
            break;
        }
      }

      /* If all tasks are done, return. */
      if (client->running_tasks == 0)
        break;

      if (client->new_tasks > 0 && ! (client->options.no_new))
        continue;

      if (client->options.non_blocking)
      {
        /* Let the caller wait for activity. */
        client->state= GEARMAN_CLIENT_STATE_IDLE;

        return GEARMAN_IO_WAIT;
      }

      /* Wait for activity on one of the connections. */
      ret= gearman_wait(&client->universal);
      if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
      {
        client->state= GEARMAN_CLIENT_STATE_IDLE;

        return ret;
      }
    }

    break;
  }

  client->state= GEARMAN_CLIENT_STATE_IDLE;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_client_run_tasks(gearman_client_st *client)
{
  gearman_return_t rc;

  _push_non_blocking(client);

  rc= _client_run_tasks(client);

  _pop_non_blocking(client);

  return rc;
}

/*
 * Static Definitions
 */

static gearman_client_st *_client_allocate(gearman_client_st *client, bool is_clone)
{
  if (client == NULL)
  {
    client= new (std::nothrow) gearman_client_st;
    if (client == NULL)
      return NULL;

    client->options.allocated= true;
  }
  else
  {
    client->options.allocated= false;
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
    gearman_universal_st *check;

    check= gearman_universal_create(&client->universal, NULL);
    if (check == NULL)
    {
      gearman_client_free(client);
      return NULL;
    }
  }

  return client;
}

static gearman_return_t _client_add_server(const char *host, in_port_t port,
                                           void *context)
{
  return gearman_client_add_server(static_cast<gearman_client_st *>(context), host, port);
}

static gearman_return_t _client_run_task(gearman_client_st *client,
                                         gearman_task_st *task)
{
  switch(task->state)
  {
  case GEARMAN_TASK_STATE_NEW:
    if (task->client->universal.con_list == NULL)
    {
      client->new_tasks--;
      client->running_tasks--;
      gearman_universal_set_error(&client->universal,
                                  GEARMAN_NO_SERVERS,
                                  "_client_run_task",
                                  "no servers added");
      return GEARMAN_NO_SERVERS;
    }

    for (task->con= task->client->universal.con_list; task->con != NULL;
         task->con= task->con->next)
    {
      if (task->con->send_state == GEARMAN_CON_SEND_STATE_NONE)
        break;
    }

    if (task->con == NULL)
    {
      client->options.no_new= true;
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
      gearman_return_t ret;
      ret= gearman_connection_send(task->con, &(task->send),
                                   client->new_tasks == 0 ? true : false);

      if (ret == GEARMAN_SUCCESS)
      {
        break;
      }
      else if (ret == GEARMAN_IO_WAIT)
      {
        task->state= GEARMAN_TASK_STATE_SUBMIT;
        return GEARMAN_IO_WAIT;
      }
      else if (ret != GEARMAN_SUCCESS)
      {
        /* Increment this since the job submission failed. */
        task->con->created_id++;

        if (ret == GEARMAN_COULD_NOT_CONNECT)
        {
          for (task->con= task->con->next; task->con != NULL;
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
      if (task->func.workload_fn == NULL)
      {
        gearman_universal_set_error(&client->universal,
                                    GEARMAN_NEED_WORKLOAD_FN,
                                    "_client_run_task",
                                    "workload size > 0, but no data pointer or workload_fn was given");
        return GEARMAN_NEED_WORKLOAD_FN;
      }

  case GEARMAN_TASK_STATE_WORKLOAD:
      gearman_return_t ret= task->func.workload_fn(task);
      if (ret != GEARMAN_SUCCESS)
      {
        task->state= GEARMAN_TASK_STATE_WORKLOAD;
        return ret;
      }
    }

    client->options.no_new= false;
    task->state= GEARMAN_TASK_STATE_WORK;
    return gearman_connection_set_events(task->con, POLLIN);

  case GEARMAN_TASK_STATE_WORK:
    if (task->recv->command == GEARMAN_COMMAND_JOB_CREATED)
    {
      snprintf(task->job_handle, GEARMAN_JOB_HANDLE_SIZE, "%.*s",
               int(task->recv->arg_size[0]),
               static_cast<char *>(task->recv->arg[0]));

  case GEARMAN_TASK_STATE_CREATED:
      if (task->func.created_fn != NULL)
      {
        gearman_return_t ret= task->func.created_fn(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_CREATED;
          return ret;
        }
      }

      if (task->send.command == GEARMAN_COMMAND_SUBMIT_JOB_BG ||
          task->send.command == GEARMAN_COMMAND_SUBMIT_JOB_HIGH_BG ||
          task->send.command == GEARMAN_COMMAND_SUBMIT_JOB_LOW_BG ||
          task->send.command == GEARMAN_COMMAND_SUBMIT_JOB_EPOCH)
      {
        break;
      }
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_DATA)
    {
  case GEARMAN_TASK_STATE_DATA:
      if (task->func.data_fn != NULL)
      {
        gearman_return_t ret= task->func.data_fn(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_DATA;
          return ret;
        }
      }
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_WARNING)
    {
  case GEARMAN_TASK_STATE_WARNING:
      if (task->func.warning_fn != NULL)
      {
        gearman_return_t ret= task->func.warning_fn(task);
        if (ret != GEARMAN_SUCCESS)
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
      if (task->func.status_fn != NULL)
      {
        gearman_return_t ret= task->func.status_fn(task);
        if (ret != GEARMAN_SUCCESS)
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
  case GEARMAN_TASK_STATE_COMPLETE:
      if (task->func.complete_fn != NULL)
      {
        gearman_return_t ret= task->func.complete_fn(task);
        if (ret != GEARMAN_SUCCESS)
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
      if (task->func.exception_fn != NULL)
      {
        gearman_return_t ret= task->func.exception_fn(task);
        if (ret != GEARMAN_SUCCESS)
        {
          task->state= GEARMAN_TASK_STATE_EXCEPTION;
          return ret;
        }
      }
    }
    else if (task->recv->command == GEARMAN_COMMAND_WORK_FAIL)
    {
  case GEARMAN_TASK_STATE_FAIL:
      if (task->func.fail_fn != NULL)
      {
        gearman_return_t ret= task->func.fail_fn(task);
        if (ret != GEARMAN_SUCCESS)
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
    gearman_task_free(task);

  return GEARMAN_SUCCESS;
}

static void *_client_do(gearman_client_st *client, gearman_command_t command,
                        const char *function_name, size_t function_name_length,
                        const char *unique, size_t unique_length,
                        const void *workload, size_t workload_size,
                        size_t *result_size, gearman_return_t *ret_ptr)
{
  gearman_task_st do_task, *do_task_ptr;
  do_task_ptr= add_task(client, &do_task, NULL, command,
                        function_name, function_name_length,
                        unique, unique_length,
                        workload, workload_size,
                        0);
  if (not do_task_ptr)
  {
    if (ret_ptr)
      *ret_ptr= gearman_universal_error_code(&client->universal);
    return NULL;
  }

  do_task_ptr->func= gearman_actions_do_default();

  do {
    *ret_ptr= gearman_client_run_tasks(client);
  } while (*ret_ptr == GEARMAN_PAUSE);

  const char *returnable= NULL;
  if (*ret_ptr == GEARMAN_TIMEOUT or *ret_ptr == GEARMAN_NO_SERVERS)
  {
    *result_size= 0;
  }
  else if (*ret_ptr == GEARMAN_SUCCESS and do_task_ptr->result_rc == GEARMAN_SUCCESS)
  {
    *ret_ptr= do_task_ptr->result_rc;
    *result_size= gearman_string_length(do_task_ptr->result_ptr);
    if (*result_size)
      returnable= gearman_string_value_take(do_task_ptr->result_ptr);
  }
  else
  {
    *ret_ptr= do_task_ptr->result_rc;
    *result_size= 0;
  }

  assert(client->task_list);
  gearman_task_free(&do_task);
  client->new_tasks= 0;
  client->running_tasks= 0;

  return static_cast<void *>(const_cast<char *>(returnable));
}

static gearman_return_t _client_do_background(gearman_client_st *client,
                                              gearman_command_t command,
                                              const char *function_name,
                                              size_t function_name_length,
                                              const char *unique,
                                              size_t unique_length,
                                              const void *workload,
                                              size_t workload_size,
                                              char *job_handle)
{
  gearman_return_t ret;

  gearman_task_st do_task, *do_task_ptr;
  do_task_ptr= add_task(client, &do_task, client, command,
                        function_name, function_name_length,
                        unique, unique_length,
                        workload, workload_size,
                        0);
  if (not do_task_ptr)
  {
    return gearman_universal_error_code(&client->universal);
  }

  gearman_task_clear_fn(do_task_ptr);

  ret= gearman_client_run_tasks(client);
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
