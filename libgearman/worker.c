/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Worker Definitions
 */

#include "common.h"

/**
  Private structure.
*/
struct _worker_function_st
{
  struct {
    bool packet_in_use:1;
    bool change:1;
    bool remove:1;
  } options;
  struct _worker_function_st *next;
  struct _worker_function_st *prev;
  char *function_name;
  size_t function_length;
  gearman_worker_fn *worker_fn;
  void *context;
  gearman_packet_st packet;
};

/**
 * @addtogroup gearman_worker_static Static Worker Declarations
 * @ingroup gearman_worker
 * @{
 */

static inline struct _worker_function_st *_function_exist(gearman_worker_st *worker, const char *function_name, size_t function_length)
{
  struct _worker_function_st *function;

  for (function= worker->function_list; function != NULL;
       function= function->next)
  {
    if (function_length == function->function_length)
    {
      if (! memcmp(function_name, function->function_name, function_length))
        break;
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
static gearman_return_t _worker_add_server(const char *host, in_port_t port,
                                           void *context);

/**
 * Allocate and add a function to the register list.
 */
static gearman_return_t _worker_function_create(gearman_worker_st *worker,
                                                const char *function_name,
                                                size_t function_length,
                                                uint32_t timeout,
                                                gearman_worker_fn *worker_fn,
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

  if (worker == NULL)
    return NULL;

  if (_worker_packet_init(worker) != GEARMAN_SUCCESS)
  {
    gearman_worker_free(worker);
    return NULL;
  }

  return worker;
}

gearman_worker_st *gearman_worker_clone(gearman_worker_st *worker,
                                        const gearman_worker_st *from)
{
  gearman_universal_st *check;

  if (! from)
  {
    return _worker_allocate(worker, false);
  }

  worker= _worker_allocate(worker, true);

  if (worker == NULL)
  {
    return worker;
  }

  worker->options.non_blocking= from->options.non_blocking;
  worker->options.grab_job_in_use= from->options.grab_job_in_use;
  worker->options.pre_sleep_in_use= from->options.pre_sleep_in_use;
  worker->options.work_job_in_use= from->options.work_job_in_use;
  worker->options.change= from->options.change;
  worker->options.grab_uniq= from->options.grab_uniq;
  worker->options.timeout_return= from->options.timeout_return;

  check= gearman_universal_clone(&(worker->universal), &from->universal);
  if (check == NULL)
  {
    gearman_worker_free(worker);
    return NULL;
  }

  if (_worker_packet_init(worker) != GEARMAN_SUCCESS)
  {
    gearman_worker_free(worker);
    return NULL;
  }

  return worker;
}

void gearman_worker_free(gearman_worker_st *worker)
{
  if (worker->options.packet_init)
  {
    gearman_packet_free(&(worker->grab_job));
    gearman_packet_free(&(worker->pre_sleep));
  }

  if (worker->job != NULL)
    gearman_job_free(worker->job);

  if (worker->options.work_job_in_use)
    gearman_job_free(&(worker->work_job));

  if (worker->work_result != NULL)
  {
    if ((&worker->universal)->workload_free_fn == NULL)
      free(worker->work_result);
    else
    {
      (&worker->universal)->workload_free_fn(worker->work_result,
                                (void *)(&worker->universal)->workload_free_context);
    }
  }

  while (worker->function_list != NULL)
    _worker_function_free(worker, worker->function_list);

  gearman_job_free_all(worker);

  if ((&worker->universal) != NULL)
    gearman_universal_free((&worker->universal));

  if (worker->options.allocated)
    free(worker);
}

const char *gearman_worker_error(gearman_worker_st *worker)
{
  return gearman_universal_error((&worker->universal));
}

int gearman_worker_errno(gearman_worker_st *worker)
{
  return gearman_universal_errno((&worker->universal));
}

gearman_worker_options_t gearman_worker_options(const gearman_worker_st *worker)
{
  gearman_worker_options_t options;
  memset(&options, 0, sizeof(gearman_worker_options_t));

  if (worker->options.allocated)
    options|= GEARMAN_WORKER_ALLOCATED;
  if (worker->options.non_blocking)
    options|= GEARMAN_WORKER_NON_BLOCKING;
  if (worker->options.packet_init)
    options|= GEARMAN_WORKER_PACKET_INIT;
  if (worker->options.grab_job_in_use)
    options|= GEARMAN_WORKER_GRAB_JOB_IN_USE;
  if (worker->options.pre_sleep_in_use)
    options|= GEARMAN_WORKER_PRE_SLEEP_IN_USE;
  if (worker->options.work_job_in_use)
    options|= GEARMAN_WORKER_WORK_JOB_IN_USE;
  if (worker->options.change)
    options|= GEARMAN_WORKER_CHANGE;
  if (worker->options.grab_uniq)
    options|= GEARMAN_WORKER_GRAB_UNIQ;
  if (worker->options.timeout_return)
    options|= GEARMAN_WORKER_TIMEOUT_RETURN;

  return options;
}

void gearman_worker_set_options(gearman_worker_st *worker,
                                gearman_worker_options_t options)
{
  gearman_worker_options_t usable_options[]= {
    GEARMAN_WORKER_NON_BLOCKING,
    GEARMAN_WORKER_GRAB_UNIQ,
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
  if (options & GEARMAN_WORKER_NON_BLOCKING)
  {
    gearman_universal_add_options((&worker->universal), GEARMAN_NON_BLOCKING);
    worker->options.non_blocking= true;
  }

  if (options & GEARMAN_WORKER_GRAB_UNIQ)
  {
    worker->grab_job.command= GEARMAN_COMMAND_GRAB_JOB_UNIQ;
    (void)gearman_packet_pack_header(&(worker->grab_job));
    worker->options.grab_uniq= true;
  }

  if (options & GEARMAN_WORKER_TIMEOUT_RETURN)
  {
    worker->options.timeout_return= true;
  }
}

void gearman_worker_remove_options(gearman_worker_st *worker,
                                   gearman_worker_options_t options)
{
  if (options & GEARMAN_WORKER_NON_BLOCKING)
  {
    gearman_universal_remove_options((&worker->universal), GEARMAN_NON_BLOCKING);
    worker->options.non_blocking= false;
  }

  if (options & GEARMAN_WORKER_TIMEOUT_RETURN)
  {
    worker->options.timeout_return= false;
  }

  if (options & GEARMAN_WORKER_GRAB_UNIQ)
  {
    worker->grab_job.command= GEARMAN_COMMAND_GRAB_JOB;
    (void)gearman_packet_pack_header(&(worker->grab_job));
    worker->options.grab_uniq= false;
  }
}

int gearman_worker_timeout(gearman_worker_st *worker)
{
  return gearman_universal_timeout((&worker->universal));
}

void gearman_worker_set_timeout(gearman_worker_st *worker, int timeout)
{
  gearman_worker_add_options(worker, GEARMAN_WORKER_TIMEOUT_RETURN);
  gearman_universal_set_timeout((&worker->universal), timeout);
}

void *gearman_worker_context(const gearman_worker_st *worker)
{
  return worker->context;
}

void gearman_worker_set_context(gearman_worker_st *worker, void *context)
{
  worker->context= context;
}

void gearman_worker_set_log_fn(gearman_worker_st *worker,
                               gearman_log_fn *function, void *context,
                               gearman_verbose_t verbose)
{
  gearman_set_log_fn((&worker->universal), function, context, verbose);
}

void gearman_worker_set_workload_malloc_fn(gearman_worker_st *worker,
                                           gearman_malloc_fn *function,
                                           void *context)
{
  gearman_set_workload_malloc_fn((&worker->universal), function, context);
}

void gearman_worker_set_workload_free_fn(gearman_worker_st *worker,
                                         gearman_free_fn *function,
                                         void *context)
{
  gearman_set_workload_free_fn((&worker->universal), function, context);
}

gearman_return_t gearman_worker_add_server(gearman_worker_st *worker,
                                           const char *host, in_port_t port)
{
  if (gearman_connection_create_args((&worker->universal), NULL, host, port) == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_worker_add_servers(gearman_worker_st *worker,
                                            const char *servers)
{
  return gearman_parse_servers(servers, _worker_add_server, worker);
}

void gearman_worker_remove_servers(gearman_worker_st *worker)
{
  gearman_free_all_cons((&worker->universal));
}

gearman_return_t gearman_worker_wait(gearman_worker_st *worker)
{
  return gearman_wait((&worker->universal));
}

gearman_return_t gearman_worker_register(gearman_worker_st *worker,
                                         const char *function_name,
                                         uint32_t timeout)
{
  return _worker_function_create(worker, function_name, strlen(function_name), timeout, NULL, NULL);
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
  struct _worker_function_st *function;
  gearman_return_t ret;
  const void *args[1];
  size_t args_size[1];

  function= _function_exist(worker, function_name, function_length);

  if (function == NULL || function->options.remove)
    return GEARMAN_NO_REGISTERED_FUNCTION;

  gearman_packet_free(&(function->packet));

  args[0]= function->function_name;
  args_size[0]= function->function_length;
  ret= gearman_packet_create_args((&worker->universal), &(function->packet),
                                  GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_CANT_DO,
                                  args, args_size, 1);
  if (ret != GEARMAN_SUCCESS)
  {
    function->options.packet_in_use= false;
    return ret;
  }

  function->options.change= true;
  function->options.remove= true;

  worker->options.change= true;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_worker_unregister(gearman_worker_st *worker,
                                           const char *function_name)
{
  return _worker_unregister(worker, function_name, strlen(function_name));
}

gearman_return_t gearman_worker_unregister_all(gearman_worker_st *worker)
{
  gearman_return_t ret;
  struct _worker_function_st *function;
  uint32_t count= 0;

  if (worker->function_list == NULL)
    return GEARMAN_NO_REGISTERED_FUNCTIONS;


  /* Lets find out if we have any functions left that are valid */
  for (function= worker->function_list; function != NULL;
       function= function->next)
  {
    if (function->options.remove == false)
      count++;
  }

  if (count == 0)
    return GEARMAN_NO_REGISTERED_FUNCTIONS;

  gearman_packet_free(&(worker->function_list->packet));

  ret= gearman_packet_create_args((&worker->universal),
                                  &(worker->function_list->packet),
                                  GEARMAN_MAGIC_REQUEST,
                                  GEARMAN_COMMAND_RESET_ABILITIES,
                                  NULL, NULL, 0);
  if (ret != GEARMAN_SUCCESS)
  {
    worker->function_list->options.packet_in_use= false;

    return ret;
  }

  while (worker->function_list->next != NULL)
    _worker_function_free(worker, worker->function_list->next);

  worker->function_list->options.change= true;
  worker->function_list->options.remove= true;

  worker->options.change= true;

  return GEARMAN_SUCCESS;
}

gearman_job_st *gearman_worker_grab_job(gearman_worker_st *worker,
                                        gearman_job_st *job,
                                        gearman_return_t *ret_ptr)
{
  struct _worker_function_st *function;
  uint32_t active;

  while (1)
  {
    switch (worker->state)
    {
    case GEARMAN_WORKER_STATE_START:
      /* If there are any new functions changes, send them now. */
      if (worker->options.change)
      {
        worker->function= worker->function_list;
        while (worker->function != NULL)
        {
          if (! (worker->function->options.change))
          {
            worker->function= worker->function->next;
            continue;
          }

          for (worker->con= (&worker->universal)->con_list; worker->con != NULL;
               worker->con= worker->con->next)
          {
            if (worker->con->fd == -1)
              continue;

    case GEARMAN_WORKER_STATE_FUNCTION_SEND:
            *ret_ptr= gearman_connection_send(worker->con, &(worker->function->packet),
                                       true);
            if (*ret_ptr != GEARMAN_SUCCESS)
            {
              if (*ret_ptr == GEARMAN_IO_WAIT)
                worker->state= GEARMAN_WORKER_STATE_FUNCTION_SEND;
              else if (*ret_ptr == GEARMAN_LOST_CONNECTION)
                continue;

              return NULL;
            }
          }

          if (worker->function->options.remove)
          {
            function= worker->function->prev;
            _worker_function_free(worker, worker->function);
            if (function == NULL)
              worker->function= worker->function_list;
            else
              worker->function= function;
          }
          else
          {
            worker->function->options.change= false;
            worker->function= worker->function->next;
          }
        }

        worker->options.change= false;
      }

      if (worker->function_list == NULL)
      {
        gearman_universal_set_error((&worker->universal), "gearman_worker_grab_job",
                          "no functions have been registered");
        *ret_ptr= GEARMAN_NO_REGISTERED_FUNCTIONS;
        return NULL;
      }

      for (worker->con= (&worker->universal)->con_list; worker->con != NULL;
           worker->con= worker->con->next)
      {
        /* If the connection to the job server is not active, start it. */
        if (worker->con->fd == -1)
        {
          for (worker->function= worker->function_list;
               worker->function != NULL;
               worker->function= worker->function->next)
          {
    case GEARMAN_WORKER_STATE_CONNECT:
            *ret_ptr= gearman_connection_send(worker->con, &(worker->function->packet),
                                       true);
            if (*ret_ptr != GEARMAN_SUCCESS)
            {
              if (*ret_ptr == GEARMAN_IO_WAIT)
                worker->state= GEARMAN_WORKER_STATE_CONNECT;
              else if (*ret_ptr == GEARMAN_COULD_NOT_CONNECT ||
                       *ret_ptr == GEARMAN_LOST_CONNECTION)
              {
                break;
              }

              return NULL;
            }
          }

          if (*ret_ptr == GEARMAN_COULD_NOT_CONNECT)
            continue;
        }

    case GEARMAN_WORKER_STATE_GRAB_JOB_SEND:
        if (worker->con->fd == -1)
          continue;

        *ret_ptr= gearman_connection_send(worker->con, &(worker->grab_job), true);
        if (*ret_ptr != GEARMAN_SUCCESS)
        {
          if (*ret_ptr == GEARMAN_IO_WAIT)
            worker->state= GEARMAN_WORKER_STATE_GRAB_JOB_SEND;
          else if (*ret_ptr == GEARMAN_LOST_CONNECTION)
            continue;

          return NULL;
        }

        if (worker->job == NULL)
        {
          worker->job= gearman_job_create(worker, job);
          if (worker->job == NULL)
          {
            *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
            return NULL;
          }
        }

        while (1)
        {
    case GEARMAN_WORKER_STATE_GRAB_JOB_RECV:
          (void)gearman_connection_recv(worker->con, &(worker->job->assigned), ret_ptr,
                                 true);
          if (*ret_ptr != GEARMAN_SUCCESS)
          {
            if (*ret_ptr == GEARMAN_IO_WAIT)
              worker->state= GEARMAN_WORKER_STATE_GRAB_JOB_RECV;
            else
            {
              gearman_job_free(worker->job);
              worker->job= NULL;

              if (*ret_ptr == GEARMAN_LOST_CONNECTION)
                break;
            }

            return NULL;
          }

          if (worker->job->assigned.command == GEARMAN_COMMAND_JOB_ASSIGN ||
              worker->job->assigned.command == GEARMAN_COMMAND_JOB_ASSIGN_UNIQ)
          {
            worker->job->options.assigned_in_use= true;
            worker->job->con= worker->con;
            worker->state= GEARMAN_WORKER_STATE_GRAB_JOB_SEND;
            job= worker->job;
            worker->job= NULL;
            return job;
          }

          if (worker->job->assigned.command == GEARMAN_COMMAND_NO_JOB)
          {
            gearman_packet_free(&(worker->job->assigned));
            break;
          }

          if (worker->job->assigned.command != GEARMAN_COMMAND_NOOP)
          {
            gearman_universal_set_error((&worker->universal), "gearman_worker_grab_job",
                              "unexpected packet:%s",
                              gearman_command_info_list[worker->job->assigned.command].name);
            gearman_packet_free(&(worker->job->assigned));
            gearman_job_free(worker->job);
            worker->job= NULL;
            *ret_ptr= GEARMAN_UNEXPECTED_PACKET;
            return NULL;
          }

          gearman_packet_free(&(worker->job->assigned));
        }
      }

    case GEARMAN_WORKER_STATE_PRE_SLEEP:
      for (worker->con= (&worker->universal)->con_list; worker->con != NULL;
           worker->con= worker->con->next)
      {
        if (worker->con->fd == -1)
          continue;

        *ret_ptr= gearman_connection_send(worker->con, &(worker->pre_sleep), true);
        if (*ret_ptr != GEARMAN_SUCCESS)
        {
          if (*ret_ptr == GEARMAN_IO_WAIT)
            worker->state= GEARMAN_WORKER_STATE_PRE_SLEEP;
          else if (*ret_ptr == GEARMAN_LOST_CONNECTION)
            continue;

          return NULL;
        }
      }

      worker->state= GEARMAN_WORKER_STATE_START;

      /* Set a watch on all active connections that we sent a PRE_SLEEP to. */
      active= 0;
      for (worker->con= (&worker->universal)->con_list; worker->con != NULL;
           worker->con= worker->con->next)
      {
        if (worker->con->fd == -1)
          continue;

        *ret_ptr= gearman_connection_set_events(worker->con, POLLIN);
        if (*ret_ptr != GEARMAN_SUCCESS)
          return NULL;

        active++;
      }

      if ((&worker->universal)->options.non_blocking)
      {
        *ret_ptr= GEARMAN_NO_JOBS;
        return NULL;
      }

      if (active == 0)
      {
        if ((&worker->universal)->timeout < 0)
          usleep(GEARMAN_WORKER_WAIT_TIMEOUT * 1000);
        else
        {
          if ((&worker->universal)->timeout > 0)
            usleep((unsigned int)(&worker->universal)->timeout * 1000);

          if (worker->options.timeout_return)
          {
            gearman_universal_set_error((&worker->universal), "gearman_worker_grab_job",
                              "timeout reached");
            *ret_ptr= GEARMAN_TIMEOUT;
            return NULL;
          }
        }
      }
      else
      {
        *ret_ptr= gearman_wait((&worker->universal));
        if (*ret_ptr != GEARMAN_SUCCESS && (*ret_ptr != GEARMAN_TIMEOUT ||
            worker->options.timeout_return))
        {
          return NULL;
        }
      }

      break;

    default:
      gearman_universal_set_error((&worker->universal), "gearman_worker_grab_job",
                        "unknown state: %u", worker->state);
      *ret_ptr= GEARMAN_UNKNOWN_STATE;
      return NULL;
    }
  }
}

void gearman_job_free(gearman_job_st *job)
{
  if (job->options.assigned_in_use)
    gearman_packet_free(&(job->assigned));

  if (job->options.work_in_use)
    gearman_packet_free(&(job->work));

  if (job->worker->job_list == job)
    job->worker->job_list= job->next;
  if (job->prev != NULL)
    job->prev->next= job->next;
  if (job->next != NULL)
    job->next->prev= job->prev;
  job->worker->job_count--;

  if (job->options.allocated)
    free(job);
}

void gearman_job_free_all(gearman_worker_st *worker)
{
  while (worker->job_list != NULL)
    gearman_job_free(worker->job_list);
}

gearman_return_t gearman_worker_add_function(gearman_worker_st *worker,
                                             const char *function_name,
                                             uint32_t timeout,
                                             gearman_worker_fn *worker_fn,
                                             void *context)
{
  if (function_name == NULL)
  {
    gearman_universal_set_error((&worker->universal), "gearman_worker_add_function",
                      "function name not given");

    return GEARMAN_INVALID_FUNCTION_NAME;
  }

  if (worker_fn == NULL)
  {
    gearman_universal_set_error((&worker->universal), "gearman_worker_add_function",
                      "function not given");

    return GEARMAN_INVALID_WORKER_FUNCTION;
  }

  return _worker_function_create(worker, function_name, strlen(function_name),
                                 timeout, worker_fn,
                                 context);
}

gearman_return_t gearman_worker_work(gearman_worker_st *worker)
{
  gearman_job_st *check;
  gearman_return_t ret;

  switch (worker->work_state)
  {
  case GEARMAN_WORKER_WORK_UNIVERSAL_GRAB_JOB:
    check= gearman_worker_grab_job(worker, &(worker->work_job), &ret);
    (void)check; // @todo test this good values

    if (ret != GEARMAN_SUCCESS)
      return ret;

    for (worker->work_function= worker->function_list;
         worker->work_function != NULL;
         worker->work_function= worker->work_function->next)
    {
      if (!strcmp(gearman_job_function_name(&(worker->work_job)),
                  worker->work_function->function_name))
      {
        break;
      }
    }

    if (worker->work_function == NULL)
    {
      gearman_job_free(&(worker->work_job));
      gearman_universal_set_error((&worker->universal), "gearman_worker_work",
                        "function not found");
      return GEARMAN_INVALID_FUNCTION_NAME;
    }

    if (worker->work_function->worker_fn == NULL)
    {
      gearman_job_free(&(worker->work_job));
      gearman_universal_set_error((&worker->universal), "gearman_worker_work",
                        "no callback function supplied");
      return GEARMAN_INVALID_FUNCTION_NAME;
    }

    worker->options.work_job_in_use= true;
    worker->work_result_size= 0;

  case GEARMAN_WORKER_WORK_UNIVERSAL_FUNCTION:
    worker->work_result= worker->work_function->worker_fn(&(worker->work_job),
                                         (void *)worker->work_function->context,
                                         &(worker->work_result_size), &ret);
    if (ret == GEARMAN_WORK_FAIL)
    {
      ret= gearman_job_send_fail(&(worker->work_job));
      if (ret != GEARMAN_SUCCESS)
      {
        if (ret == GEARMAN_LOST_CONNECTION)
          break;

        worker->work_state= GEARMAN_WORKER_WORK_UNIVERSAL_FAIL;
        return ret;
      }

      break;
    }

    if (ret != GEARMAN_SUCCESS)
    {
      if (ret == GEARMAN_LOST_CONNECTION)
        break;

      worker->work_state= GEARMAN_WORKER_WORK_UNIVERSAL_FUNCTION;
      return ret;
    }

  case GEARMAN_WORKER_WORK_UNIVERSAL_COMPLETE:
    ret= gearman_job_send_complete(&(worker->work_job), worker->work_result,
                              worker->work_result_size);
    if (ret == GEARMAN_IO_WAIT)
    {
      worker->work_state= GEARMAN_WORKER_WORK_UNIVERSAL_COMPLETE;
      return ret;
    }

    if (worker->work_result != NULL)
    {
      if ((&worker->universal)->workload_free_fn == NULL)
        free(worker->work_result);
      else
      {
        (&worker->universal)->workload_free_fn(worker->work_result,
                                          (&worker->universal)->workload_free_context);
      }
      worker->work_result= NULL;
    }

    if (ret != GEARMAN_SUCCESS)
    {
      if (ret == GEARMAN_LOST_CONNECTION)
        break;

      return ret;
    }

    break;

  case GEARMAN_WORKER_WORK_UNIVERSAL_FAIL:
    ret= gearman_job_send_fail(&(worker->work_job));
    if (ret != GEARMAN_SUCCESS)
    {
      if (ret == GEARMAN_LOST_CONNECTION)
        break;

      return ret;
    }

   break;

  default:
    gearman_universal_set_error((&worker->universal), "gearman_worker_work",
                      "unknown state: %u", worker->work_state);
    return GEARMAN_UNKNOWN_STATE;
  }

  gearman_job_free(&(worker->work_job));
  worker->options.work_job_in_use= false;
  worker->work_state= GEARMAN_WORKER_WORK_UNIVERSAL_GRAB_JOB;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_worker_echo(gearman_worker_st *worker,
                                     const void *workload,
                                     size_t workload_size)
{
  return gearman_echo((&worker->universal), workload, workload_size);
}

/*
 * Static Definitions
 */

static gearman_worker_st *_worker_allocate(gearman_worker_st *worker, bool is_clone)
{
  if (worker == NULL)
  {
    worker= malloc(sizeof(gearman_worker_st));
    if (worker == NULL)
      return NULL;

    worker->options.allocated= true;
  }
  else
  {
    worker->options.allocated= false;
  }

  worker->options.non_blocking= false;
  worker->options.packet_init= false;
  worker->options.grab_job_in_use= false;
  worker->options.pre_sleep_in_use= false;
  worker->options.work_job_in_use= false;
  worker->options.change= false;
  worker->options.grab_uniq= false;
  worker->options.timeout_return= false;

  worker->state= 0;
  worker->work_state= 0;
  worker->function_count= 0;
  worker->job_count= 0;
  worker->work_result_size= 0;
  worker->con= NULL;
  worker->job= NULL;
  worker->job_list= NULL;
  worker->function= NULL;
  worker->function_list= NULL;
  worker->work_function= NULL;
  worker->work_result= NULL;

  if (! is_clone)
  {
    gearman_universal_st *check;

    check= gearman_universal_create(&worker->universal, NULL);
    if (check == NULL)
    {
      gearman_worker_free(worker);
      return NULL;
    }

    gearman_universal_set_timeout((&worker->universal), GEARMAN_WORKER_WAIT_TIMEOUT);
  }

  return worker;
}

static gearman_return_t _worker_packet_init(gearman_worker_st *worker)
{
  gearman_return_t ret;

  ret= gearman_packet_create_args((&worker->universal), &(worker->grab_job),
                                  GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_GRAB_JOB,
                                  NULL, NULL, 0);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  ret= gearman_packet_create_args((&worker->universal), &(worker->pre_sleep),
                                  GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_PRE_SLEEP,
                                  NULL, NULL, 0);
  if (ret != GEARMAN_SUCCESS)
  {
    gearman_packet_free(&(worker->grab_job));
    return ret;
  }

  worker->options.packet_init= true;

  return GEARMAN_SUCCESS;
}

static gearman_return_t _worker_add_server(const char *host, in_port_t port,
                                           void *context)
{
  return gearman_worker_add_server((gearman_worker_st *)context, host, port);
}

static gearman_return_t _worker_function_create(gearman_worker_st *worker,
                                                const char *function_name,
                                                size_t function_length,
                                                uint32_t timeout,
                                                gearman_worker_fn *worker_fn,
                                                void *context)
{
  struct _worker_function_st *function;
  gearman_return_t ret;
  char timeout_buffer[11];
  const void *args[2];
  size_t args_size[2];

  function= malloc(sizeof(struct _worker_function_st));
  if (function == NULL)
  {
    gearman_universal_set_error((&worker->universal), "_worker_function_create", "malloc");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  function->options.packet_in_use= true;
  function->options.change= true;
  function->options.remove= false;

  function->function_name= strdup(function_name);
  function->function_length= function_length;
  if (function->function_name == NULL)
  {
    free(function);
    gearman_universal_set_error((&worker->universal), "gearman_worker_add_function", "strdup");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  function->worker_fn= worker_fn;
  function->context= context;

  if (timeout > 0)
  {
    snprintf(timeout_buffer, 11, "%u", timeout);
    args[0]= function_name;
    args_size[0]= strlen(function_name) + 1;
    args[1]= timeout_buffer;
    args_size[1]= strlen(timeout_buffer);
    ret= gearman_packet_create_args((&worker->universal), &(function->packet),
                                    GEARMAN_MAGIC_REQUEST,
                                    GEARMAN_COMMAND_CAN_DO_TIMEOUT,
                                    args, args_size, 2);
  }
  else
  {
    args[0]= function->function_name;
    args_size[0]= function->function_length= function_length;
    ret= gearman_packet_create_args((&worker->universal), &(function->packet),
                                    GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_CAN_DO,
                                    args, args_size, 1);
  }
  if (ret != GEARMAN_SUCCESS)
  {
    free(function->function_name);
    free(function);
    return ret;
  }

  if (worker->function_list != NULL)
    worker->function_list->prev= function;
  function->next= worker->function_list;
  function->prev= NULL;
  worker->function_list= function;
  worker->function_count++;

  worker->options.change= true;

  return GEARMAN_SUCCESS;
}

static void _worker_function_free(gearman_worker_st *worker,
                                  struct _worker_function_st *function)
{
  if (worker->function_list == function)
    worker->function_list= function->next;
  if (function->prev != NULL)
    function->prev->next= function->next;
  if (function->next != NULL)
    function->next->prev= function->prev;
  worker->function_count--;

  if (function->options.packet_in_use)
    gearman_packet_free(&(function->packet));

  free(function->function_name);
  free(function);
}
