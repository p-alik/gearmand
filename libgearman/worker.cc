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

#include "gear_config.h"

#include <libgearman/common.h>
#include <libgearman/function/base.hpp>
#include <libgearman/function/make.hpp>

#include "libgearman/pipe.h"

#include "libgearman/assert.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

/**
 * @addtogroup gearman_worker_static Static Worker Declarations
 * @ingroup gearman_worker
 * @{
 */

static inline struct _worker_function_st *_function_exist(gearman_worker_st *worker, const char *function_name, size_t function_length)
{
  struct _worker_function_st *function;

  for (function= worker->impl()->function_list; function;
       function= function->next)
  {
    if (function_length == function->function_length)
    {
      if (memcmp(function_name, function->function_name, function_length) == 0)
      {
        break;
      }
    }
  }

  return function;
}

/**
 * Allocate a worker structure.
 */
static gearman_worker_st *_worker_allocate(gearman_worker_st *worker, bool is_clone);

/**
 * Initialize common packets for later use.
 */
static gearman_return_t _worker_packet_init(gearman_worker_st *worker);

/**
 * Callback function used when parsing server lists.
 */
static gearman_return_t _worker_add_server(const char *host, in_port_t port, void *context);

/**
 * Allocate and add a function to the register list.
 */
static gearman_return_t _worker_function_create(gearman_worker_st *worker,
                                                const char *function_name, size_t function_length,
                                                const gearman_function_t &function,
                                                uint32_t timeout,
                                                void *context);

/**
 * Free a function.
 */
static void _worker_function_free(gearman_worker_st *worker,
                                  struct _worker_function_st *function);


/** @} */

/*
 * Public Definitions
 */

gearman_worker_st *gearman_worker_create(gearman_worker_st *worker)
{
  worker= _worker_allocate(worker, false);

  if (worker)
  {
    if (gearman_failed(_worker_packet_init(worker)))
    {
      gearman_worker_free(worker);
      return NULL;
    }
  }

  return worker;
}

gearman_worker_st *gearman_worker_clone(gearman_worker_st *worker,
                                        const gearman_worker_st *source)
{
  if (source == NULL)
  {
    return gearman_worker_create(worker);
  }

  worker= _worker_allocate(worker, true);

  if (worker == NULL)
  {
    return worker;
  }

  worker->impl()->options.non_blocking= source->impl()->options.non_blocking;
  worker->impl()->options.change= source->impl()->options.change;
  worker->impl()->options.grab_uniq= source->impl()->options.grab_uniq;
  worker->impl()->options.grab_all= source->impl()->options.grab_all;
  worker->impl()->options.timeout_return= source->impl()->options.timeout_return;

  gearman_universal_clone(worker->impl()->universal, source->impl()->universal, true);

  if (gearman_failed(_worker_packet_init(worker)))
  {
    gearman_worker_free(worker);
    return NULL;
  }

  return worker;
}

void gearman_worker_free(gearman_worker_st *worker)
{
  if (worker)
  {
    if (worker->impl()->universal.wakeup_fd[0] != INVALID_SOCKET)
    {
      close(worker->impl()->universal.wakeup_fd[0]);
    }

    if (worker->impl()->universal.wakeup_fd[1] != INVALID_SOCKET)
    {
      close(worker->impl()->universal.wakeup_fd[1]);
    }

    gearman_worker_unregister_all(worker);

    if (worker->impl()->options.packet_init)
    {
      gearman_packet_free(&worker->impl()->grab_job);
      gearman_packet_free(&worker->impl()->pre_sleep);
    }

    gearman_job_free(worker->impl()->job);
    worker->impl()->work_job= NULL;

    if (worker->impl()->work_result)
    {
      gearman_free(worker->impl()->universal, worker->impl()->work_result);
    }

    while (worker->impl()->function_list)
    {
      _worker_function_free(worker, worker->impl()->function_list);
    }

    gearman_job_free_all(worker);

    gearman_universal_free(worker->impl()->universal);

    delete worker->impl();
  }
}

const char *gearman_worker_error(const gearman_worker_st *worker_shell)
{
  if (worker_shell)
  {
    return worker_shell->impl()->universal.error();
  }

  return NULL;
}

int gearman_worker_errno(gearman_worker_st *worker_shell)
{
  if (worker_shell)
  {
    return worker_shell->impl()->universal.last_errno();
  }

  return EINVAL;
}

gearman_worker_options_t gearman_worker_options(const gearman_worker_st *worker)
{
  if (worker == NULL)
  {
    return gearman_worker_options_t();
  }

  int options;
  memset(&options, 0, sizeof(gearman_worker_options_t));

  if (gearman_is_allocated(worker))
  {
    options|= int(GEARMAN_WORKER_ALLOCATED);
  }

  if (worker->impl()->options.non_blocking)
    options|= int(GEARMAN_WORKER_NON_BLOCKING);
  if (worker->impl()->options.packet_init)
    options|= int(GEARMAN_WORKER_PACKET_INIT);
  if (worker->impl()->options.change)
    options|= int(GEARMAN_WORKER_CHANGE);
  if (worker->impl()->options.grab_uniq)
    options|= int(GEARMAN_WORKER_GRAB_UNIQ);
  if (worker->impl()->options.grab_all)
    options|= int(GEARMAN_WORKER_GRAB_ALL);
  if (worker->impl()->options.timeout_return)
    options|= int(GEARMAN_WORKER_TIMEOUT_RETURN);

  return gearman_worker_options_t(options);
}

void gearman_worker_set_options(gearman_worker_st *worker,
                                gearman_worker_options_t options)
{
  if (worker == NULL)
  {
    return;
  }

  gearman_worker_options_t usable_options[]= {
    GEARMAN_WORKER_NON_BLOCKING,
    GEARMAN_WORKER_GRAB_UNIQ,
    GEARMAN_WORKER_GRAB_ALL,
    GEARMAN_WORKER_TIMEOUT_RETURN,
    GEARMAN_WORKER_MAX
  };

  gearman_worker_options_t *ptr;


  for (ptr= usable_options; *ptr != GEARMAN_WORKER_MAX ; ptr++)
  {
    if (options & *ptr)
    {
      gearman_worker_add_options(worker, *ptr);
    }
    else
    {
      gearman_worker_remove_options(worker, *ptr);
    }
  }
}

void gearman_worker_add_options(gearman_worker_st *worker,
                                gearman_worker_options_t options)
{
  if (worker == NULL)
  {
    return;
  }

  if (options & GEARMAN_WORKER_NON_BLOCKING)
  {
    gearman_universal_add_options(worker->impl()->universal, GEARMAN_NON_BLOCKING);
    worker->impl()->options.non_blocking= true;
  }

  if (options & GEARMAN_WORKER_GRAB_UNIQ)
  {
    worker->impl()->grab_job.command= GEARMAN_COMMAND_GRAB_JOB_UNIQ;
    gearman_return_t rc= gearman_packet_pack_header(&(worker->impl()->grab_job));
    (void)(rc);
    assert(gearman_success(rc));
    worker->impl()->options.grab_uniq= true;
  }

  if (options & GEARMAN_WORKER_GRAB_ALL)
  {
    worker->impl()->grab_job.command= GEARMAN_COMMAND_GRAB_JOB_ALL;
    gearman_return_t rc= gearman_packet_pack_header(&(worker->impl()->grab_job));
    (void)(rc);
    assert(gearman_success(rc));
    worker->impl()->options.grab_all= true;
  }

  if (options & GEARMAN_WORKER_TIMEOUT_RETURN)
  {
    worker->impl()->options.timeout_return= true;
  }
}

void gearman_worker_remove_options(gearman_worker_st *worker,
                                   gearman_worker_options_t options)
{
  if (worker == NULL)
  {
    return;
  }

  if (options & GEARMAN_WORKER_NON_BLOCKING)
  {
    gearman_universal_remove_options(worker->impl()->universal, GEARMAN_NON_BLOCKING);
    worker->impl()->options.non_blocking= false;
  }

  if (options & GEARMAN_WORKER_TIMEOUT_RETURN)
  {
    worker->impl()->options.timeout_return= false;
    gearman_universal_set_timeout(worker->impl()->universal, GEARMAN_WORKER_WAIT_TIMEOUT);
  }

  if (options & GEARMAN_WORKER_GRAB_UNIQ)
  {
    worker->impl()->grab_job.command= GEARMAN_COMMAND_GRAB_JOB;
    (void)gearman_packet_pack_header(&(worker->impl()->grab_job));
    worker->impl()->options.grab_uniq= false;
  }

  if (options & GEARMAN_WORKER_GRAB_ALL)
  {
    worker->impl()->grab_job.command= GEARMAN_COMMAND_GRAB_JOB;
    (void)gearman_packet_pack_header(&(worker->impl()->grab_job));
    worker->impl()->options.grab_all= false;
  }
}

int gearman_worker_timeout(gearman_worker_st *worker)
{
  if (worker == NULL)
  {
    return 0;
  }

  return gearman_universal_timeout(worker->impl()->universal);
}

void gearman_worker_set_timeout(gearman_worker_st *worker, int timeout)
{
  if (worker == NULL)
  {
    return;
  }

  gearman_worker_add_options(worker, GEARMAN_WORKER_TIMEOUT_RETURN);
  gearman_universal_set_timeout(worker->impl()->universal, timeout);
}

void *gearman_worker_context(const gearman_worker_st *worker)
{
  if (worker == NULL)
  {
    return NULL;
  }

  return worker->impl()->context;
}

void gearman_worker_set_context(gearman_worker_st *worker, void *context)
{
  if (worker == NULL)
  {
    return;
  }

  worker->impl()->context= context;
}

void gearman_worker_set_log_fn(gearman_worker_st *worker,
                               gearman_log_fn *function, void *context,
                               gearman_verbose_t verbose)
{
  gearman_set_log_fn(worker->impl()->universal, function, context, verbose);
}

void gearman_worker_set_workload_malloc_fn(gearman_worker_st *worker,
                                           gearman_malloc_fn *function,
                                           void *context)
{
  if (worker == NULL)
  {
    return;
  }

  gearman_set_workload_malloc_fn(worker->impl()->universal, function, context);
}

void gearman_worker_set_workload_free_fn(gearman_worker_st *worker,
                                         gearman_free_fn *function,
                                         void *context)
{
  if (worker == NULL)
  {
    return;
  }

  gearman_set_workload_free_fn(worker->impl()->universal, function, context);
}

gearman_return_t gearman_worker_add_server(gearman_worker_st *worker,
                                           const char *host, in_port_t port)
{
  if (worker == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  if (gearman_connection_create_args(worker->impl()->universal, host, port) == NULL)
  {
    return gearman_universal_error_code(worker->impl()->universal);
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_worker_add_servers(gearman_worker_st *worker, const char *servers)
{
  return gearman_parse_servers(servers, _worker_add_server, worker);
}

void gearman_worker_remove_servers(gearman_worker_st *worker)
{
  if (worker == NULL)
  {
    return;
  }

  gearman_free_all_cons(worker->impl()->universal);
}

gearman_return_t gearman_worker_wait(gearman_worker_st *worker)
{
  if (worker == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  return gearman_wait(worker->impl()->universal);
}

gearman_return_t gearman_worker_register(gearman_worker_st *worker,
                                         const char *function_name,
                                         uint32_t timeout)
{
  gearman_function_t null_func= gearman_function_create_null();
  return _worker_function_create(worker, function_name, strlen(function_name), null_func, timeout, NULL);
}

bool gearman_worker_function_exist(gearman_worker_st *worker,
                                   const char *function_name,
                                   size_t function_length)
{
  struct _worker_function_st *function;

  function= _function_exist(worker, function_name, function_length);

  return (function && function->options.remove == false) ? true : false;
}

static inline gearman_return_t _worker_unregister(gearman_worker_st *worker,
                                                  const char *function_name, size_t function_length)
{

  _worker_function_st *function= _function_exist(worker, function_name, function_length);

  if (function == NULL || function->options.remove)
  {
    return GEARMAN_NO_REGISTERED_FUNCTION;
  }

  if (function->options.packet_in_use)
  {
    gearman_packet_free(&(function->packet()));
    function->options.packet_in_use= false;
  }

  const void *args[1];
  size_t args_size[1];
  args[0]= function->name();
  args_size[0]= function->length();
  gearman_return_t ret= gearman_packet_create_args(worker->impl()->universal, function->packet(),
                                                   GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_CANT_DO,
                                                   args, args_size, 1);
  if (gearman_failed(ret))
  {
    function->options.packet_in_use= false;
    return ret;
  }
  function->options.packet_in_use= true;

  function->options.change= true;
  function->options.remove= true;

  worker->impl()->options.change= true;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_worker_unregister(gearman_worker_st *worker,
                                           const char *function_name)
{
  return _worker_unregister(worker, function_name, strlen(function_name));
}

gearman_return_t gearman_worker_unregister_all(gearman_worker_st *worker)
{
  struct _worker_function_st *function;
  uint32_t count= 0;

  if (worker->impl()->function_list == NULL)
  {
    return GEARMAN_NO_REGISTERED_FUNCTIONS;
  }

  /* Lets find out if we have any functions left that are valid */
  for (function= worker->impl()->function_list; function;
       function= function->next)
  {
    if (function->options.remove == false)
    {
      count++;
    }
  }

  if (count == 0)
  {
    return GEARMAN_NO_REGISTERED_FUNCTIONS;
  }

  gearman_packet_free(&(worker->impl()->function_list->packet()));

  gearman_return_t ret= gearman_packet_create_args(worker->impl()->universal,
                                                   worker->impl()->function_list->packet(),
                                                   GEARMAN_MAGIC_REQUEST,
                                                   GEARMAN_COMMAND_RESET_ABILITIES,
                                                   NULL, NULL, 0);
  if (gearman_failed(ret))
  {
    worker->impl()->function_list->options.packet_in_use= false;

    return ret;
  }

  while (worker->impl()->function_list->next)
  {
    _worker_function_free(worker, worker->impl()->function_list->next);
  }

  worker->impl()->function_list->options.change= true;
  worker->impl()->function_list->options.remove= true;

  worker->impl()->options.change= true;

  return GEARMAN_SUCCESS;
}

gearman_job_st *gearman_worker_grab_job(gearman_worker_st *worker,
                                        gearman_job_st *job,
                                        gearman_return_t *ret_ptr)
{
  struct _worker_function_st *function;
  uint32_t active;

  gearman_return_t unused;
  if (not ret_ptr)
  {
    ret_ptr= &unused;
  }

  while (1)
  {
    switch (worker->impl()->state)
    {
    case GEARMAN_WORKER_STATE_START:
      /* If there are any new functions changes, send them now. */
      if (worker->impl()->options.change)
      {
        worker->impl()->function= worker->impl()->function_list;
        while (worker->impl()->function)
        {
          if (not (worker->impl()->function->options.change))
          {
            worker->impl()->function= worker->impl()->function->next;
            continue;
          }

          for (worker->impl()->con= (&worker->impl()->universal)->con_list; worker->impl()->con;
               worker->impl()->con= worker->impl()->con->next)
          {
            if (worker->impl()->con->fd == -1)
            {
              continue;
            }

    case GEARMAN_WORKER_STATE_FUNCTION_SEND:
            *ret_ptr= worker->impl()->con->send_packet(worker->impl()->function->packet(), true);
            if (gearman_failed(*ret_ptr))
            {
              if (*ret_ptr == GEARMAN_IO_WAIT)
              {
                worker->impl()->state= GEARMAN_WORKER_STATE_FUNCTION_SEND;
              }
              else if (*ret_ptr == GEARMAN_LOST_CONNECTION)
              {
                continue;
              }

              return NULL;
            }
          }

          if (worker->impl()->function->options.remove)
          {
            function= worker->impl()->function->prev;
            _worker_function_free(worker, worker->impl()->function);
            if (function == NULL)
              worker->impl()->function= worker->impl()->function_list;
            else
              worker->impl()->function= function;
          }
          else
          {
            worker->impl()->function->options.change= false;
            worker->impl()->function= worker->impl()->function->next;
          }
        }

        worker->impl()->options.change= false;
      }

      if (not worker->impl()->function_list)
      {
        gearman_error(worker->impl()->universal, GEARMAN_NO_REGISTERED_FUNCTIONS, "no functions have been registered");
        *ret_ptr= GEARMAN_NO_REGISTERED_FUNCTIONS;
        return NULL;
      }

      for (worker->impl()->con= (&worker->impl()->universal)->con_list; worker->impl()->con;
           worker->impl()->con= worker->impl()->con->next)
      {
        /* If the connection to the job server is not active, start it. */
        if (worker->impl()->con->fd == -1)
        {
          for (worker->impl()->function= worker->impl()->function_list;
               worker->impl()->function;
               worker->impl()->function= worker->impl()->function->next)
          {
    case GEARMAN_WORKER_STATE_CONNECT:
            *ret_ptr= worker->impl()->con->send_packet(worker->impl()->function->packet(), true);
            if (gearman_failed(*ret_ptr))
            {
              if (*ret_ptr == GEARMAN_IO_WAIT)
              {
                worker->impl()->state= GEARMAN_WORKER_STATE_CONNECT;
              }
              else if (*ret_ptr == GEARMAN_COULD_NOT_CONNECT or *ret_ptr == GEARMAN_LOST_CONNECTION)
              {
                break;
              }

              return NULL;
            }
          }

          if (*ret_ptr == GEARMAN_COULD_NOT_CONNECT)
          {
            continue;
          }
        }

    case GEARMAN_WORKER_STATE_GRAB_JOB_SEND:
        if (worker->impl()->con->fd == -1)
          continue;

        *ret_ptr= worker->impl()->con->send_packet(worker->impl()->grab_job, true);
        if (gearman_failed(*ret_ptr))
        {
          if (*ret_ptr == GEARMAN_IO_WAIT)
          {
            worker->impl()->state= GEARMAN_WORKER_STATE_GRAB_JOB_SEND;
          }
          else if (*ret_ptr == GEARMAN_LOST_CONNECTION)
          {
            continue;
          }

          return NULL;
        }

        if (not worker->impl()->job)
        {
          worker->impl()->job= gearman_job_create(worker, job);
          if (not worker->impl()->job)
          {
            *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
            return NULL;
          }
        }

        while (1)
        {
    case GEARMAN_WORKER_STATE_GRAB_JOB_RECV:
          assert(worker->impl()->job);
          (void)worker->impl()->con->receiving(worker->impl()->job->assigned, *ret_ptr, true);

          if (gearman_failed(*ret_ptr))
          {
            if (*ret_ptr == GEARMAN_IO_WAIT)
            {
              worker->impl()->state= GEARMAN_WORKER_STATE_GRAB_JOB_RECV;
            }
            else
            {
              gearman_job_free(worker->impl()->job);
              worker->impl()->job= NULL;

              if (*ret_ptr == GEARMAN_LOST_CONNECTION)
              {
                break;
              }
            }

            return NULL;
          }

          if (worker->impl()->job->assigned.command == GEARMAN_COMMAND_JOB_ASSIGN or
              worker->impl()->job->assigned.command == GEARMAN_COMMAND_JOB_ASSIGN_ALL or
              worker->impl()->job->assigned.command == GEARMAN_COMMAND_JOB_ASSIGN_UNIQ)
          {
            worker->impl()->job->options.assigned_in_use= true;
            worker->impl()->job->con= worker->impl()->con;
            worker->impl()->state= GEARMAN_WORKER_STATE_GRAB_JOB_SEND;
            job= worker->impl()->job;
            worker->impl()->job= NULL;

            return job;
          }

          if (worker->impl()->job->assigned.command == GEARMAN_COMMAND_NO_JOB or
              worker->impl()->job->assigned.command == GEARMAN_COMMAND_OPTION_RES)
          {
            gearman_packet_free(&(worker->impl()->job->assigned));
            break;
          }

          if (worker->impl()->job->assigned.command != GEARMAN_COMMAND_NOOP)
          {
            gearman_universal_set_error(worker->impl()->universal, GEARMAN_UNEXPECTED_PACKET, GEARMAN_AT,
                                        "unexpected packet:%s",
                                        gearman_command_info(worker->impl()->job->assigned.command)->name);
            gearman_packet_free(&(worker->impl()->job->assigned));
            gearman_job_free(worker->impl()->job);
            worker->impl()->job= NULL;
            *ret_ptr= GEARMAN_UNEXPECTED_PACKET;
            return NULL;
          }

          gearman_packet_free(&(worker->impl()->job->assigned));
        }
      }

    case GEARMAN_WORKER_STATE_PRE_SLEEP:
      for (worker->impl()->con= (&worker->impl()->universal)->con_list; worker->impl()->con;
           worker->impl()->con= worker->impl()->con->next)
      {
        if (worker->impl()->con->fd == INVALID_SOCKET)
        {
          continue;
        }

        *ret_ptr= worker->impl()->con->send_packet(worker->impl()->pre_sleep, true);
        if (gearman_failed(*ret_ptr))
        {
          if (*ret_ptr == GEARMAN_IO_WAIT)
          {
            worker->impl()->state= GEARMAN_WORKER_STATE_PRE_SLEEP;
          }
          else if (*ret_ptr == GEARMAN_LOST_CONNECTION)
          {
            continue;
          }

          return NULL;
        }
      }

      worker->impl()->state= GEARMAN_WORKER_STATE_START;

      /* Set a watch on all active connections that we sent a PRE_SLEEP to. */
      active= 0;
      for (worker->impl()->con= worker->impl()->universal.con_list; worker->impl()->con; worker->impl()->con= worker->impl()->con->next)
      {
        if (worker->impl()->con->fd == INVALID_SOCKET)
        {
          continue;
        }

        worker->impl()->con->set_events(POLLIN);
        active++;
      }

      if ((&worker->impl()->universal)->options.non_blocking)
      {
        *ret_ptr= GEARMAN_NO_JOBS;
        return NULL;
      }

      if (active == 0)
      {
        if (worker->impl()->universal.timeout < 0)
        {
          gearman_nap(GEARMAN_WORKER_WAIT_TIMEOUT);
        }
        else
        {
          if (worker->impl()->universal.timeout > 0)
          {
            gearman_nap(worker->impl()->universal);
          }

          if (worker->impl()->options.timeout_return)
          {
            *ret_ptr= gearman_error(worker->impl()->universal, GEARMAN_TIMEOUT, "Option timeout return reached");

            return NULL;
          }
        }
      }
      else
      {
        *ret_ptr= gearman_wait(worker->impl()->universal);
        if (gearman_failed(*ret_ptr) and (*ret_ptr != GEARMAN_TIMEOUT or worker->impl()->options.timeout_return))
        {
          return NULL;
        }
      }

      break;
    }
  }
}

void gearman_job_free_all(gearman_worker_st *worker)
{
  while (worker->impl()->job_list)
  {
    gearman_job_free(worker->impl()->job_list);
  }
}

gearman_return_t gearman_worker_add_function(gearman_worker_st *worker,
                                             const char *function_name,
                                             uint32_t timeout,
                                             gearman_worker_fn *worker_fn,
                                             void *context)
{
  if (worker == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  if (function_name == NULL)
  {
    return gearman_error(worker->impl()->universal, GEARMAN_INVALID_ARGUMENT, "function name not given");
  }

  if (worker_fn == NULL)
  {
    return gearman_error(worker->impl()->universal, GEARMAN_INVALID_ARGUMENT, "function not given");
  }
  gearman_function_t local= gearman_function_create_v1(worker_fn);

  return _worker_function_create(worker,
                                 function_name, strlen(function_name),
                                 local,
                                 timeout,
                                 context);
}

gearman_return_t gearman_worker_define_function(gearman_worker_st *worker,
                                                const char *function_name, const size_t function_name_length,
                                                const gearman_function_t function,
                                                const uint32_t timeout,
                                                void *context)
{
  if (worker == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  if (function_name == NULL or function_name_length == 0)
  {
    return gearman_error(worker->impl()->universal, GEARMAN_INVALID_ARGUMENT, "function name not given");
  }

  return _worker_function_create(worker,
                                 function_name, function_name_length,
                                 function,
                                 timeout,
                                 context);

  return GEARMAN_INVALID_ARGUMENT;
}

void gearman_worker_reset_error(gearman_worker_st *worker)
{
  if (worker and worker->impl())
  {
    universal_reset_error(worker->impl()->universal);
  }
}

gearman_return_t gearman_worker_work(gearman_worker_st *worker)
{
  bool shutdown= false;

  if (worker == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  universal_reset_error(worker->impl()->universal);

  switch (worker->impl()->work_state)
  {
  case GEARMAN_WORKER_WORK_UNIVERSAL_GRAB_JOB:
    {
      gearman_return_t ret;
      worker->impl()->work_job= gearman_worker_grab_job(worker, NULL, &ret);

      if (gearman_failed(ret))
      {
        if (ret == GEARMAN_COULD_NOT_CONNECT)
        {
          gearman_reset(worker->impl()->universal);
        }
        return ret;
      }
      assert(worker->impl()->work_job);

      for (worker->impl()->work_function= worker->impl()->function_list;
           worker->impl()->work_function;
           worker->impl()->work_function= worker->impl()->work_function->next)
      {
        if (not strcmp(gearman_job_function_name(worker->impl()->work_job),
                       worker->impl()->work_function->function_name))
        {
          break;
        }
      }

      if (not worker->impl()->work_function)
      {
        gearman_job_free(worker->impl()->work_job);
        worker->impl()->work_job= NULL;
        return gearman_error(worker->impl()->universal, GEARMAN_INVALID_FUNCTION_NAME, "Function not found");
      }

      if (not worker->impl()->work_function->has_callback())
      {
        gearman_job_free(worker->impl()->work_job);
        worker->impl()->work_job= NULL;
        return gearman_error(worker->impl()->universal, GEARMAN_INVALID_FUNCTION_NAME, "Neither a gearman_worker_fn, or gearman_function_fn callback was supplied");
      }

      worker->impl()->work_result_size= 0;
    }

  case GEARMAN_WORKER_WORK_UNIVERSAL_FUNCTION:
    {
      switch (worker->impl()->work_function->callback(worker->impl()->work_job,
                                              static_cast<void *>(worker->impl()->work_function->context)))
      {
      case GEARMAN_FUNCTION_INVALID_ARGUMENT:
        worker->impl()->work_job->error_code= gearman_error(worker->impl()->universal, GEARMAN_INVALID_ARGUMENT, "worker returned an invalid response, gearman_return_t");
      case GEARMAN_FUNCTION_FATAL:
        if (gearman_job_send_fail_fin(worker->impl()->work_job) == GEARMAN_LOST_CONNECTION) // If we fail this, we have no connection, @note this causes us to lose the current error
        {
          worker->impl()->work_job->error_code= GEARMAN_LOST_CONNECTION;
          break;
        }
        worker->impl()->work_state= GEARMAN_WORKER_WORK_UNIVERSAL_FAIL;
        return worker->impl()->work_job->error_code;

      case GEARMAN_FUNCTION_ERROR: // retry 
        gearman_reset(worker->impl()->universal);
        worker->impl()->work_job->error_code= GEARMAN_LOST_CONNECTION;
        break;

      case GEARMAN_FUNCTION_SHUTDOWN:
        shutdown= true;

      case GEARMAN_FUNCTION_SUCCESS:
        break;
      }

      if (worker->impl()->work_job->error_code == GEARMAN_LOST_CONNECTION)
      {
        break;
      }
    }

  case GEARMAN_WORKER_WORK_UNIVERSAL_COMPLETE:
    {
      worker->impl()->work_job->error_code= gearman_job_send_complete_fin(worker->impl()->work_job,
                                                                  worker->impl()->work_result, worker->impl()->work_result_size);
      if (worker->impl()->work_job->error_code == GEARMAN_IO_WAIT)
      {
        worker->impl()->work_state= GEARMAN_WORKER_WORK_UNIVERSAL_COMPLETE;
        return gearman_error(worker->impl()->universal, worker->impl()->work_job->error_code,
                             "A failure occurred after worker had successful complete, unless gearman_job_send_complete() was called directly by worker, client has not been informed of success.");
      }

      if (worker->impl()->work_result)
      {
        gearman_free(worker->impl()->universal, worker->impl()->work_result);
        worker->impl()->work_result= NULL;
      }

      // If we lost the connection, we retry the work, otherwise we error
      if (worker->impl()->work_job->error_code == GEARMAN_LOST_CONNECTION)
      {
        break;
      }
      else if (worker->impl()->work_job->error_code == GEARMAN_SHUTDOWN)
      { }
      else if (gearman_failed(worker->impl()->work_job->error_code))
      {
        worker->impl()->work_state= GEARMAN_WORKER_WORK_UNIVERSAL_FAIL;

        return worker->impl()->work_job->error_code;
      }
    }
    break;

  case GEARMAN_WORKER_WORK_UNIVERSAL_FAIL:
    {
      if (gearman_failed(worker->impl()->work_job->error_code= gearman_job_send_fail_fin(worker->impl()->work_job)))
      {
        if (worker->impl()->work_job->error_code == GEARMAN_LOST_CONNECTION)
        {
          break;
        }

        return worker->impl()->work_job->error_code;
      }
    }
    break;
  }

  gearman_job_free(worker->impl()->work_job);
  worker->impl()->work_job= NULL;

  worker->impl()->work_state= GEARMAN_WORKER_WORK_UNIVERSAL_GRAB_JOB;

  if (shutdown)
  {
    return GEARMAN_SHUTDOWN;
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_worker_echo(gearman_worker_st *worker,
                                     const void *workload,
                                     size_t workload_size)
{
  if (worker == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  return gearman_echo(worker->impl()->universal, workload, workload_size);
}

/*
 * Static Definitions
 */

static gearman_worker_st *_worker_allocate(gearman_worker_st *worker_shell, bool is_clone)
{
  Worker *worker= new (std::nothrow) Worker(worker_shell);
  if (worker)
  {
    if (is_clone == false)
    {
#if 0
      gearman_universal_initialize(worker->impl()->universal);
      gearman_universal_set_timeout(worker->impl()->universal, GEARMAN_WORKER_WAIT_TIMEOUT);
#endif
    }

    if (setup_shutdown_pipe(worker->universal.wakeup_fd) == false)
    {
      delete worker;
      return NULL;
    }

    return worker->shell();
  }
#if defined(DEBUG) && DEBUG
  perror("new Worker");
#endif

  return NULL;
}

static gearman_return_t _worker_packet_init(gearman_worker_st *worker)
{
  gearman_return_t ret= gearman_packet_create_args(worker->impl()->universal, worker->impl()->grab_job,
                                                   GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_GRAB_JOB_ALL,
                                                   NULL, NULL, 0);
  if (gearman_failed(ret))
  {
    return ret;
  }

  ret= gearman_packet_create_args(worker->impl()->universal, worker->impl()->pre_sleep,
                                  GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_PRE_SLEEP,
                                  NULL, NULL, 0);
  if (gearman_failed(ret))
  {
    gearman_packet_free(&(worker->impl()->grab_job));
    return ret;
  }

  worker->impl()->options.packet_init= true;

  return GEARMAN_SUCCESS;
}

static gearman_return_t _worker_add_server(const char *host, in_port_t port, void *context)
{
  return gearman_worker_add_server(static_cast<gearman_worker_st *>(context), host, port);
}

static gearman_return_t _worker_function_create(gearman_worker_st *worker,
                                                const char *function_name, size_t function_length,
                                                const gearman_function_t &function_arg,
                                                uint32_t timeout,
                                                void *context)
{
  const void *args[2];
  size_t args_size[2];

  if (worker == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  if (function_length == 0 or function_name == NULL or function_length > GEARMAN_FUNCTION_MAX_SIZE)
  {
    if (function_length > GEARMAN_FUNCTION_MAX_SIZE)
    {
      gearman_error(worker->impl()->universal, GEARMAN_INVALID_ARGUMENT, "function name longer then GEARMAN_MAX_FUNCTION_SIZE");
    } 
    else
    {
      gearman_error(worker->impl()->universal, GEARMAN_INVALID_ARGUMENT, "invalid function");
    }

    return GEARMAN_INVALID_ARGUMENT;
  }

  _worker_function_st *function= make(worker->impl()->universal._namespace, function_name, function_length, function_arg, context);
  if (function == NULL)
  {
    gearman_perror(worker->impl()->universal, "_worker_function_st::new()");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  gearman_return_t ret;
  if (timeout > 0)
  {
    char timeout_buffer[11];
    snprintf(timeout_buffer, sizeof(timeout_buffer), "%u", timeout);
    args[0]= function->name();
    args_size[0]= function->length() + 1;
    args[1]= timeout_buffer;
    args_size[1]= strlen(timeout_buffer);
    ret= gearman_packet_create_args(worker->impl()->universal, function->packet(),
                                    GEARMAN_MAGIC_REQUEST,
                                    GEARMAN_COMMAND_CAN_DO_TIMEOUT,
                                    args, args_size, 2);
  }
  else
  {
    args[0]= function->name();
    args_size[0]= function->length();
    ret= gearman_packet_create_args(worker->impl()->universal, function->packet(),
                                    GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_CAN_DO,
                                    args, args_size, 1);
  }

  if (gearman_failed(ret))
  {
    delete function;

    return ret;
  }

  if (worker->impl()->function_list)
  {
    worker->impl()->function_list->prev= function;
  }

  function->next= worker->impl()->function_list;
  function->prev= NULL;
  worker->impl()->function_list= function;
  worker->impl()->function_count++;

  worker->impl()->options.change= true;

  return GEARMAN_SUCCESS;
}

static void _worker_function_free(gearman_worker_st *worker,
                                  struct _worker_function_st *function)
{
  if (worker->impl()->function_list == function)
  {
    worker->impl()->function_list= function->next;
  }

  if (function->prev)
  {
    function->prev->next= function->next;
  }

  if (function->next)
  {
    function->next->prev= function->prev;
  }
  worker->impl()->function_count--;

  delete function;
}

gearman_return_t gearman_worker_set_memory_allocators(gearman_worker_st *worker,
                                                      gearman_malloc_fn *malloc_fn,
                                                      gearman_free_fn *free_fn,
                                                      gearman_realloc_fn *realloc_fn,
                                                      gearman_calloc_fn *calloc_fn,
                                                      void *context)
{
  if (worker == NULL)
  {
    return GEARMAN_INVALID_ARGUMENT;
  }

  return gearman_set_memory_allocator(worker->impl()->universal.allocator, malloc_fn, free_fn, realloc_fn, calloc_fn, context);
}

bool gearman_worker_set_server_option(gearman_worker_st *worker_shell, const char *option_arg, size_t option_arg_size)
{
  if (worker_shell and worker_shell->impl())
  {
    gearman_string_t option= { option_arg, option_arg_size };
    return gearman_request_option(worker_shell->impl()->universal, option);
  }

  return false;
}

void gearman_worker_set_namespace(gearman_worker_st *self, const char *namespace_key, size_t namespace_key_size)
{
  if (self)
  {
    gearman_universal_set_namespace(self->impl()->universal, namespace_key, namespace_key_size);
  }
}

gearman_id_t gearman_worker_id(gearman_worker_st *self)
{
  if (self == NULL)
  {
    gearman_id_t handle= { INVALID_SOCKET, INVALID_SOCKET };
    return handle;
  }

  return gearman_universal_id(self->impl()->universal);
}

gearman_worker_st *gearman_job_clone_worker(gearman_job_st *job)
{
  return gearman_worker_clone(NULL, job->worker);
}

gearman_return_t gearman_worker_set_identifier(gearman_worker_st *worker,
                                               const char *id, size_t id_size)
{
  return gearman_set_identifier(worker->impl()->universal, id, id_size);
}

const char *gearman_worker_namespace(gearman_worker_st *self)
{
  return gearman_univeral_namespace(self->impl()->universal);
}
