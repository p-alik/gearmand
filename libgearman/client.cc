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

#include <config.h>

#include <libgearman/common.h>
#include <libgearman/log.hpp>

#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>

/*
  Allocate a client structure.
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
    if (client == NULL)
    {
      return NULL;
    }

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

  if (is_clone == false)
  {
    gearman_universal_initialize(client->universal);
  }

  return client;
}

/**
 * Callback function used when parsing server lists.
 */
static gearman_return_t _client_add_server(const char *host, in_port_t port,
                                           void *context)
{
  return gearman_client_add_server(static_cast<gearman_client_st *>(context), host, port);
}


/**
 * Real do function.
 */
static void *_client_do(gearman_client_st *client, gearman_command_t command,
                        const char *function_name,
                        const char *unique,
                        const void *workload_str, size_t workload_size,
                        size_t *result_size, gearman_return_t *ret_ptr)
{
  gearman_return_t unused;
  if (ret_ptr == NULL)
  {
    ret_ptr= &unused;
  }

  universal_reset_error(client->universal);

  size_t unused_size;
  if (result_size == NULL)
  {
    result_size= &unused_size;
  }
  *result_size= 0;

  if (client == NULL)
  {
    *ret_ptr= GEARMAN_INVALID_ARGUMENT;
    return NULL;
  }

  gearman_string_t function= { gearman_string_param_cstr(function_name) };
  gearman_unique_t local_unique= gearman_unique_make(unique, unique ? strlen(unique) : 0);
  gearman_string_t workload= { static_cast<const char*>(workload_str), workload_size };


  gearman_task_st do_task;
  gearman_task_st *do_task_ptr= add_task(*client, &do_task, NULL, command,
                                         function,
                                         local_unique,
                                         workload,
                                         time_t(0),
                                         gearman_actions_do_default());
  if (do_task_ptr == NULL)
  {
    *ret_ptr= gearman_universal_error_code(client->universal);
    return NULL;
  }
  do_task_ptr->type= GEARMAN_TASK_KIND_DO;

  gearman_return_t ret;
  do {
    ret= gearman_client_run_tasks(client);
  } while (gearman_continue(ret));

  // gearman_client_run_tasks failed
  assert(client->task_list); // Programmer error, we should always have the task that we used for do

  char *returnable= NULL;
  if (gearman_failed(ret))
  {
    if (ret == GEARMAN_COULD_NOT_CONNECT)
    { }
    else
    {
      gearman_error(client->universal, ret, "occured during gearman_client_run_tasks()");
    }

    *ret_ptr= ret;
    *result_size= 0;
  }
  else if (gearman_success(ret) and do_task_ptr->result_rc == GEARMAN_SUCCESS)
  {
    *ret_ptr= do_task_ptr->result_rc;
    if (do_task_ptr->result_ptr)
    {
      if (gearman_has_allocator(client->universal))
      {
        gearman_string_t result= gearman_result_string(do_task_ptr->result_ptr);
        returnable= static_cast<char *>(gearman_malloc(client->universal, gearman_size(result) +1));
        if (not returnable)
        {
          gearman_error(client->universal, GEARMAN_MEMORY_ALLOCATION_FAILURE, "custom workload_fn failed to allocate memory");
          *result_size= 0;
        }
        else // NULL terminate
        {
          memcpy(returnable, gearman_c_str(result), gearman_size(result));
          returnable[gearman_size(result)]= 0;
          *result_size= gearman_size(result);
        }
      }
      else
      {
        gearman_string_t result= gearman_result_take_string(do_task_ptr->result_ptr);
        *result_size= gearman_size(result);
        returnable= const_cast<char *>(gearman_c_str(result));
      }
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

  gearman_task_free(&do_task);
  client->new_tasks= 0;
  client->running_tasks= 0;

  return returnable;
}

/*
  Real background do function.
*/
static gearman_return_t _client_do_background(gearman_client_st *client,
                                              gearman_command_t command,
                                              gearman_string_t &function,
                                              gearman_unique_t &unique,
                                              gearman_string_t &workload,
                                              gearman_job_handle_t job_handle)
{
  if (client == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  universal_reset_error(client->universal);

  if (gearman_size(function) == 0)
  {
    return gearman_error(client->universal, GEARMAN_INVALID_ARGUMENT, "function argument was empty");
  }

  client->_do_handle[0]= 0; // Reset the job_handle we store in client

  gearman_task_st do_task, *do_task_ptr;
  do_task_ptr= add_task(*client, &do_task, 
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
  do_task_ptr->type= GEARMAN_TASK_KIND_DO;

  gearman_return_t ret;
  do {
    ret= gearman_client_run_tasks(client);
    
    // If either of the following is ever true, we will end up in an
    // infinite loop
    assert(ret != GEARMAN_IN_PROGRESS and ret != GEARMAN_JOB_EXISTS);

  } while (gearman_continue(ret));

  if (job_handle)
  {
    strncpy(job_handle, do_task.job_handle, GEARMAN_JOB_HANDLE_SIZE);
  }
  strncpy(client->_do_handle, do_task.job_handle, GEARMAN_JOB_HANDLE_SIZE);
  client->new_tasks= 0;
  client->running_tasks= 0;
  gearman_task_free(&do_task);

  return ret;
}


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
  client->_do_handle[0]= 0;

  gearman_universal_clone(client->universal, from->universal);

  return client;
}

void gearman_client_free(gearman_client_st *client)
{
  if (client == NULL)
  {
    return;
  }

  gearman_client_task_free_all(client);

  gearman_universal_free(client->universal);

  if (client->options.allocated)
  {
    delete client;
  }
}

const char *gearman_client_error(const gearman_client_st *client)
{
  if (client == NULL)
  {
    return NULL;
  }

  return gearman_universal_error(client->universal);
}

gearman_return_t gearman_client_error_code(const gearman_client_st *client)
{
  if (client == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  return gearman_universal_error_code(client->universal);
}

int gearman_client_errno(const gearman_client_st *client)
{
  if (client == NULL)
  {
    return 0;
  }

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

bool gearman_client_has_option(gearman_client_st *client,
                                gearman_client_options_t option)
{
  if (client == NULL)
  {
    return false;
  }

  switch (option)
  {
  case GEARMAN_CLIENT_ALLOCATED:
    return client->options.allocated;

  case GEARMAN_CLIENT_NON_BLOCKING:
    return client->options.non_blocking;

  case GEARMAN_CLIENT_UNBUFFERED_RESULT:
    return client->options.unbuffered_result;

  case GEARMAN_CLIENT_NO_NEW:
    return client->options.no_new;

  case GEARMAN_CLIENT_FREE_TASKS:
    return client->options.free_tasks;

  default:
  case GEARMAN_CLIENT_TASK_IN_USE:
  case GEARMAN_CLIENT_MAX:
        return false;
  }
}

void gearman_client_set_options(gearman_client_st *client,
                                gearman_client_options_t options)
{
  if (client == NULL)
    return;

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
  if (client == NULL)
    return;

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
  if (client == NULL)
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
  if (client == NULL)
  {
    return;
  }

  gearman_universal_set_timeout(client->universal, timeout);
}

void *gearman_client_context(const gearman_client_st *client)
{
  if (client == NULL)
  {
    return NULL;
  }

  return const_cast<void *>(client->context);
}

void gearman_client_set_context(gearman_client_st *client, void *context)
{
  if (client == NULL)
  {
    return;
  }

  client->context= context;
}

void gearman_client_set_log_fn(gearman_client_st *client,
                               gearman_log_fn *function, void *context,
                               gearman_verbose_t verbose)
{
  if (client == NULL)
  {
    return;
  }

  gearman_set_log_fn(client->universal, function, context, verbose);
}

void gearman_client_set_workload_malloc_fn(gearman_client_st *client,
                                           gearman_malloc_fn *function,
                                           void *context)
{
  if (client == NULL)
  {
    return;
  }

  gearman_set_workload_malloc_fn(client->universal, function, context);
}

void gearman_client_set_workload_free_fn(gearman_client_st *client, gearman_free_fn *function, void *context)
{
  if (client == NULL)
  {
    return;
  }

  gearman_set_workload_free_fn(client->universal, function, context);
}

gearman_return_t gearman_client_add_server(gearman_client_st *client,
                                           const char *host, in_port_t port)
{
  if (client == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  if (gearman_connection_create_args(client->universal, host, port) == false)
  {
    assert(client->universal.error.rc != GEARMAN_SUCCESS);
    return gearman_universal_error_code(client->universal);
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
  if (client == NULL)
  {
    return;
  }

  gearman_free_all_cons(client->universal);
}

gearman_return_t gearman_client_wait(gearman_client_st *client)
{
  if (client == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

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
  if (client == NULL)
  {
    return 0;
  }

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
  if (self == NULL)
  {
    errno= EINVAL;
    return NULL;
  }

  return self->_do_handle;
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
                                              gearman_job_handle_t job_handle)
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
                                                   gearman_job_handle_t job_handle)
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
                                                  gearman_job_handle_t job_handle)
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
                                           const gearman_job_handle_t job_handle,
                                           bool *is_known, bool *is_running,
                                           uint32_t *numerator,
                                           uint32_t *denominator)
{
  gearman_return_t ret;

  if (client == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  universal_reset_error(client->universal);

  gearman_task_st do_task;
  gearman_task_st *do_task_ptr= gearman_client_add_task_status(client, &do_task, client,
                                                               job_handle, &ret);
  if (gearman_failed(ret))
  {
    return ret;
  }
  assert(do_task_ptr);
  do_task_ptr->type= GEARMAN_TASK_KIND_DO;

  gearman_task_clear_fn(do_task_ptr);

  do {
    ret= gearman_client_run_tasks(client);
    
    // If either of the following is ever true, we will end up in an
    // infinite loop
    assert(ret != GEARMAN_IN_PROGRESS and ret != GEARMAN_JOB_EXISTS);

  } while (gearman_continue(ret));

  // @note we don't know if our task was run or not, we just know something
  // happened.

  if (gearman_success(ret))
  {
    if (is_known)
    {
      *is_known= do_task.options.is_known;
    }

    if (is_running)
    {
      *is_running= do_task.options.is_running;
    }

    if (numerator)
    {
      *numerator= do_task.numerator;
    }

    if (denominator)
    {
      *denominator= do_task.denominator;
    }

    if (is_known == false and is_running == false)
    {
      if (do_task.options.is_running) 
      {
        ret= GEARMAN_IN_PROGRESS;
      }
      else if (do_task.options.is_known)
      {
        ret= GEARMAN_JOB_EXISTS;
      }
    }
  }
  else
  {
    if (is_known)
    {
      *is_known= false;
    }

    if (is_running)
    {
      *is_running= false;
    }

    if (numerator)
    {
      *numerator= 0;
    }

    if (denominator)
    {
      *denominator= 0;
    }
  }
  gearman_task_free(do_task_ptr);

  return ret;
}

gearman_return_t gearman_client_echo(gearman_client_st *client,
                                     const void *workload,
                                     size_t workload_size)
{
  if (client == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  return gearman_echo(client->universal, workload, workload_size);
}

void gearman_client_task_free_all(gearman_client_st *client)
{
  if (client == NULL)
  {
    return;
  }

  while (client->task_list)
  {
    gearman_task_free(client->task_list);
  }
}

void gearman_client_set_task_context_free_fn(gearman_client_st *client,
                                             gearman_task_context_free_fn *function)
{
  if (client == NULL)
  {
    return;
  }

  client->task_context_free_fn= function;

}

gearman_return_t gearman_client_set_memory_allocators(gearman_client_st *client,
                                                      gearman_malloc_fn *malloc_fn,
                                                      gearman_free_fn *free_fn,
                                                      gearman_realloc_fn *realloc_fn,
                                                      gearman_calloc_fn *calloc_fn,
                                                      void *context)
{
  if (client == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  return gearman_set_memory_allocator(client->universal.allocator, malloc_fn, free_fn, realloc_fn, calloc_fn, context);
}



gearman_task_st *gearman_client_add_task(gearman_client_st *client,
                                         gearman_task_st *task,
                                         void *context,
                                         const char *function,
                                         const char *unique,
                                         const void *workload, size_t workload_size,
                                         gearman_return_t *ret_ptr)
{
  gearman_return_t unused;
  if (ret_ptr == NULL)
  {
    ret_ptr= &unused;
  }

  if (client == NULL)
  {
    *ret_ptr= GEARMAN_INVALID_ARGUMENT;
    return NULL;
  }

  return add_task(*client, task,
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
  gearman_return_t unused;
  if (ret_ptr == NULL)
  {
    ret_ptr= &unused;
  }

  if (client == NULL)
  {
    *ret_ptr= GEARMAN_INVALID_ARGUMENT;
    return NULL;
  }

  return add_task(*client, task, context,
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
  gearman_return_t unused;
  if (ret_ptr == NULL)
  {
    ret_ptr= &unused;
  }

  if (client == NULL)
  {
    *ret_ptr= GEARMAN_INVALID_ARGUMENT;
    return NULL;
  }

  return add_task(*client, task, context, GEARMAN_COMMAND_SUBMIT_JOB_LOW,
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
  gearman_return_t unused;
  if (ret_ptr == NULL)
  {
    ret_ptr= &unused;
  }

  if (client == NULL)
  {
    *ret_ptr= GEARMAN_INVALID_ARGUMENT;
    return NULL;
  }

  return add_task(*client, task, context, GEARMAN_COMMAND_SUBMIT_JOB_BG,
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
  gearman_return_t unused;
  if (ret_ptr == NULL)
  {
    ret_ptr= &unused;
  }

  if (client == NULL)
  {
    *ret_ptr= GEARMAN_INVALID_ARGUMENT;
    return NULL;
  }

  return add_task(*client, task, context,
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
  gearman_return_t unused;
  if (ret_ptr == NULL)
  {
    ret_ptr= &unused;
  }

  if (client == NULL)
  {
    *ret_ptr= GEARMAN_INVALID_ARGUMENT;
    return NULL;
  }

  return add_task(*client, task, context,
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
                                                const gearman_job_handle_t job_handle,
                                                gearman_return_t *ret_ptr)
{
  const void *args[1];
  size_t args_size[1];

  gearman_return_t unused;
  if (ret_ptr == NULL)
  {
    ret_ptr= &unused;
  }

  if (client == NULL)
  {
    *ret_ptr= GEARMAN_INVALID_ARGUMENT;
    return NULL;
  }

  if ((task= gearman_task_internal_create(client, task)) == NULL)
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
  if (client == NULL)
  {
    return;
  }

  client->actions.workload_fn= function;
}

void gearman_client_set_created_fn(gearman_client_st *client,
                                   gearman_created_fn *function)
{
  if (client == NULL)
  {
    return;
  }

  client->actions.created_fn= function;
}

void gearman_client_set_data_fn(gearman_client_st *client,
                                gearman_data_fn *function)
{
  if (client == NULL)
  {
    return;
  }

  client->actions.data_fn= function;
}

void gearman_client_set_warning_fn(gearman_client_st *client,
                                   gearman_warning_fn *function)
{
  if (client == NULL)
  {
    return;
  }

  client->actions.warning_fn= function;
}

void gearman_client_set_status_fn(gearman_client_st *client,
                                  gearman_universal_status_fn *function)
{
  if (client == NULL)
  {
    return;
  }

  client->actions.status_fn= function;
}

void gearman_client_set_complete_fn(gearman_client_st *client,
                                    gearman_complete_fn *function)
{
  if (client == NULL)
  {
    return;
  }

  client->actions.complete_fn= function;
}

void gearman_client_set_exception_fn(gearman_client_st *client,
                                     gearman_exception_fn *function)
{
  if (client == NULL)
  {
    return;
  }

  client->actions.exception_fn= function;
}

void gearman_client_set_fail_fn(gearman_client_st *client,
                                gearman_fail_fn *function)
{
  if (client == NULL)
  {
    return;
  }

  client->actions.fail_fn= function;
}

void gearman_client_clear_fn(gearman_client_st *client)
{
  if (client == NULL)
  {
    return;
  }

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

static inline void _push_blocking(gearman_client_st *client)
{
  client->universal.options.stored_non_blocking= client->universal.options.non_blocking;
  client->universal.options.non_blocking= false;
}

static inline void _pop_blocking(gearman_client_st *client)
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
          {
            continue;
          }

  case GEARMAN_CLIENT_STATE_NEW:
          if (client->task == NULL)
          {
            client->state= GEARMAN_CLIENT_STATE_IDLE;
            break;
          }

          assert_msg(client == client->task->client, "Programmer error, client and task member client are not the same");
          gearman_return_t local_ret= _client_run_task(client->task);
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
            if (client->task->con != client->con or
                (client->task->state != GEARMAN_TASK_STATE_SUBMIT and
                 client->task->state != GEARMAN_TASK_STATE_WORKLOAD))
            {
              continue;
            }

  case GEARMAN_CLIENT_STATE_SUBMIT:
            if (client->task == NULL)
            {
              client->state= GEARMAN_CLIENT_STATE_IDLE;
              break;
            }
            assert_msg(client == client->task->client, "Programmer error, client and task member client are not the same");
            gearman_return_t local_ret= _client_run_task(client->task);
            if (local_ret == GEARMAN_COULD_NOT_CONNECT)
            {
              client->state= GEARMAN_CLIENT_STATE_IDLE;
              return local_ret;
            }
            else if (gearman_failed(local_ret) and local_ret != GEARMAN_IO_WAIT)
            {
              client->state= GEARMAN_CLIENT_STATE_SUBMIT;
              return local_ret;
            }
          }

          /* Connection errors are fatal. */
          if (client->con->revents & (POLLERR | POLLHUP | POLLNVAL))
          {
            gearman_error(client->universal, GEARMAN_LOST_CONNECTION, "detected lost connection in _client_run_tasks()");
            client->con->close_socket();
            client->state= GEARMAN_CLIENT_STATE_IDLE;
            return GEARMAN_LOST_CONNECTION;
          }
        }

        if ((client->con->revents & POLLIN) == 0)
        {
          continue;
        }

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
                    (client->task->state == GEARMAN_TASK_STATE_DATA or
                     client->task->state == GEARMAN_TASK_STATE_COMPLETE))
                {
                  break;
                }
              }

              /*
                Someone has set GEARMAN_CLIENT_UNBUFFERED_RESULT but hasn't setup the client to fetch data correctly.
                Fatal error :(
              */
              return gearman_universal_set_error(client->universal, GEARMAN_INVALID_ARGUMENT, GEARMAN_AT,
                                                 "client created with GEARMAN_CLIENT_UNBUFFERED_RESULT, but was not setup to use it. %s", __func__);
            }
            else
            {
              /* Read the next packet, without buffering the data part. */
              client->task= NULL;
              (void)client->con->receiving(client->con->_packet, ret, false);
            }
          }
          else
          {
            /* Read the next packet, buffering the data part. */
            client->task= NULL;
            (void)client->con->receiving(client->con->_packet, ret, true);
          }

          if (client->task == NULL)
          {
            assert(ret != GEARMAN_MAX_RETURN);

            /* Check the return of the gearman_connection_recv() calls above. */
            if (gearman_failed(ret))
            {
              if (ret == GEARMAN_IO_WAIT)
              {
                break;
              }

              client->state= GEARMAN_CLIENT_STATE_IDLE;
              return ret;
            }

            client->con->options.packet_in_use= true;

            /* We have a packet, see which task it belongs to. */
            for (client->task= client->task_list; client->task;
                 client->task= client->task->next)
            {
              if (client->task->con != client->con)
              {
                continue;
              }

              gearman_log_debug(&client->universal, "Got %s", gearman_strcommand(client->con->_packet.command));
              if (client->con->_packet.command == GEARMAN_COMMAND_JOB_CREATED)
              {
                if (client->task->created_id != client->con->created_id)
                {
                  continue;
                }

                /* New job created, drop through below and notify task. */
                client->con->created_id++;
              }
              else if (client->con->_packet.command == GEARMAN_COMMAND_ERROR)
              {
                gearman_universal_set_error(client->universal, GEARMAN_SERVER_ERROR, GEARMAN_AT,
                                            "%s:%.*s",
                                            static_cast<char *>(client->con->_packet.arg[0]),
                                            int(client->con->_packet.arg_size[1]),
                                            static_cast<char *>(client->con->_packet.arg[1]));

                /* 
                  Packet cleanup copied from "Clean up the packet" below, and must
                  remain in sync with its reference.
                */
                gearman_packet_free(&(client->con->_packet));
                client->con->options.packet_in_use= false;

                /* This step copied from _client_run_tasks() above: */
                /* Increment this value because new job created then failed. */
                client->con->created_id++;

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
              client->con->free_private_packet();
              continue;
            }

            client->task->recv= &(client->con->_packet);
          }

  case GEARMAN_CLIENT_STATE_PACKET:
          /* Let task process job created or result packet. */
          assert_msg(client == client->task->client, "Programmer error, client and task member client are not the same");
          gearman_return_t local_ret= _client_run_task(client->task);

          if (local_ret == GEARMAN_IO_WAIT)
          {
            break;
          }

          if (gearman_failed(local_ret))
          {
            client->state= GEARMAN_CLIENT_STATE_PACKET;
            return local_ret;
          }

          /* Clean up the packet. */
          client->con->free_private_packet();

          /* If all tasks are done, return. */
          if (client->running_tasks == 0)
          {
            break;
          }
        }
      }

      /* If all tasks are done, return. */
      if (client->running_tasks == 0)
      {
        break;
      }

      if (client->new_tasks > 0 and ! (client->options.no_new))
      {
        continue;
      }

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
  if (client == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  if (client->task_list == NULL) // We are immediatly successful if all tasks are completed
  {
    return GEARMAN_SUCCESS;
  }


  _push_non_blocking(client);

  gearman_return_t rc= _client_run_tasks(client);

  _pop_non_blocking(client);

  if (rc == GEARMAN_COULD_NOT_CONNECT)
  {
    gearman_reset(client->universal);
  }

  return rc;
}

gearman_return_t gearman_client_run_block_tasks(gearman_client_st *client)
{
  if (client == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  if (not client->task_list) // We are immediatly successful if all tasks are completed
  {
    return GEARMAN_SUCCESS;
  }


  _push_blocking(client);

  gearman_return_t rc= _client_run_tasks(client);

  _pop_blocking(client);

  if (gearman_failed(rc))
  {
    if (rc == GEARMAN_COULD_NOT_CONNECT)
    {
      gearman_reset(client->universal);
    }

    assert(gearman_universal_error_code(client->universal) == rc);
  }

  return rc;
}

/*
 * Static Definitions
 */

bool gearman_client_compare(const gearman_client_st *first, const gearman_client_st *second)
{
  if (first == NULL or second == NULL)
  {
    return false;
  }

  if (strcmp(first->universal.con_list->host, second->universal.con_list->host))
  {
    return false;
  }

  if (first->universal.con_list->port != second->universal.con_list->port)
  {
    return false;
  }

  return true;
}

bool gearman_client_set_server_option(gearman_client_st *self, const char *option_arg, size_t option_arg_size)
{
  if (self == NULL)
  {
    return false;
  }

  gearman_string_t option= { option_arg, option_arg_size };

  return gearman_request_option(self->universal, option);
}

void gearman_client_set_namespace(gearman_client_st *self, const char *namespace_key, size_t namespace_key_size)
{
  if (self == NULL)
  {
    return;
  }

  gearman_universal_set_namespace(self->universal, namespace_key, namespace_key_size);
}

gearman_return_t gearman_client_set_identifier(gearman_client_st *client,
                                               const char *id, size_t id_size)
{
  return gearman_set_identifier(client->universal, id, id_size);
}

