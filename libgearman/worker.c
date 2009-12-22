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
 * @addtogroup gearman_worker_static Static Worker Declarations
 * @ingroup gearman_worker
 * @{
 */

/**
 * Allocate a worker structure.
 */
static gearman_worker_st *_worker_allocate(gearman_worker_st *worker);

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
static gearman_return_t _worker_function_add(gearman_worker_st *worker,
                                             const char *function_name,
                                             uint32_t timeout,
                                             gearman_worker_fn *worker_fn,
                                             const void *context);

/**
 * Free a function.
 */
static void _worker_function_free(gearman_worker_st *worker,
                                  gearman_worker_function_st *function);

/**
 * Initialize a job structure.
 */
static gearman_job_st *_job_create(gearman_worker_st *worker,
                                   gearman_job_st *job);

/** @} */

/*
 * Public Definitions
 */

gearman_worker_st *gearman_worker_create(gearman_worker_st *worker)
{
  worker= _worker_allocate(worker);
  if (worker == NULL)
    return NULL;

  worker->gearman= gearman_state_create(&(worker->gearman_state_static));
  if (worker->gearman == NULL)
  {
    gearman_worker_free(worker);
    return NULL;
  }

  gearman_set_timeout(worker->gearman, GEARMAN_WORKER_WAIT_TIMEOUT);

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
  if (from == NULL)
    return NULL;

  worker= _worker_allocate(worker);
  if (worker == NULL)
    return NULL;

  worker->options|= (from->options &
                     (gearman_worker_options_t)~GEARMAN_WORKER_ALLOCATED);

  worker->gearman= gearman_state_clone(&(worker->gearman_state_static), from->gearman);
  if (worker->gearman == NULL)
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
  if (worker->options & GEARMAN_WORKER_PACKET_INIT)
  {
    gearman_packet_free(&(worker->grab_job));
    gearman_packet_free(&(worker->pre_sleep));
  }

  if (worker->job != NULL)
    gearman_job_free(worker->job);

  if (worker->options & GEARMAN_WORKER_WORK_JOB_IN_USE)
    gearman_job_free(&(worker->work_job));

  if (worker->work_result != NULL)
  {
    if (worker->gearman->workload_free_fn == NULL)
      free(worker->work_result);
    else
    {
      worker->gearman->workload_free_fn(worker->work_result,
                                (void *)worker->gearman->workload_free_context);
    }
  }

  while (worker->function_list != NULL)
    _worker_function_free(worker, worker->function_list);

  gearman_job_free_all(worker);

  if (worker->gearman != NULL)
    gearman_state_free(worker->gearman);

  if (worker->options & GEARMAN_WORKER_ALLOCATED)
    free(worker);
}

const char *gearman_worker_error(gearman_worker_st *worker)
{
  return gearman_state_error(worker->gearman);
}

int gearman_worker_errno(gearman_worker_st *worker)
{
  return gearman_state_errno(worker->gearman);
}

gearman_worker_options_t gearman_worker_options(const gearman_worker_st *worker)
{
  return worker->options;
}

void gearman_worker_set_options(gearman_worker_st *worker,
                                gearman_worker_options_t options)
{
  /* Call this to catch any special triggers. */
  gearman_worker_add_options(worker, options);

  worker->options= options;
}

void gearman_worker_add_options(gearman_worker_st *worker,
                                gearman_worker_options_t options)
{
  if (options & GEARMAN_WORKER_NON_BLOCKING)
    gearman_add_options(worker->gearman, GEARMAN_NON_BLOCKING);

  if (options & GEARMAN_WORKER_GRAB_UNIQ)
  {
    worker->grab_job.command= GEARMAN_COMMAND_GRAB_JOB_UNIQ;
    (void)gearman_packet_pack_header(&(worker->grab_job));
  }

  worker->options|= options;
}

void gearman_worker_remove_options(gearman_worker_st *worker,
                                   gearman_worker_options_t options)
{
  if (options & GEARMAN_WORKER_NON_BLOCKING)
    gearman_remove_options(worker->gearman, GEARMAN_NON_BLOCKING);

  if (options & GEARMAN_WORKER_GRAB_UNIQ)
  {
    worker->grab_job.command= GEARMAN_COMMAND_GRAB_JOB;
    (void)gearman_packet_pack_header(&(worker->grab_job));
  }

  worker->options&= ~options;
}

int gearman_worker_timeout(gearman_worker_st *worker)
{
  return gearman_timeout(worker->gearman);
}

void gearman_worker_set_timeout(gearman_worker_st *worker, int timeout)
{
  gearman_worker_add_options(worker, GEARMAN_WORKER_TIMEOUT_RETURN);
  gearman_set_timeout(worker->gearman, timeout);
}

void *gearman_worker_context(const gearman_worker_st *worker)
{
  return (void *)(worker->context);
}

void gearman_worker_set_context(gearman_worker_st *worker, const void *context)
{
  worker->context= context;
}

void gearman_worker_set_log_fn(gearman_worker_st *worker,
                               gearman_log_fn *function, const void *context,
                               gearman_verbose_t verbose)
{
  gearman_set_log_fn(worker->gearman, function, context, verbose);
}

void gearman_worker_set_event_watch_fn(gearman_worker_st *worker,
                                       gearman_event_watch_fn *function,
                                       const void *context)
{
  gearman_set_event_watch_fn(worker->gearman, function, context);
}

void gearman_worker_set_workload_malloc_fn(gearman_worker_st *worker,
                                           gearman_malloc_fn *function,
                                           const void *context)
{
  gearman_set_workload_malloc_fn(worker->gearman, function, context);
}

void gearman_worker_set_workload_free_fn(gearman_worker_st *worker,
                                         gearman_free_fn *function,
                                         const void *context)
{
  gearman_set_workload_free_fn(worker->gearman, function, context);
}

gearman_return_t gearman_worker_add_server(gearman_worker_st *worker,
                                           const char *host, in_port_t port)
{
  if (gearman_add_con_args(worker->gearman, NULL, host, port) == NULL)
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
  gearman_free_all_cons(worker->gearman);
}

gearman_return_t gearman_worker_wait(gearman_worker_st *worker)
{
  return gearman_wait(worker->gearman);
}

gearman_return_t gearman_worker_register(gearman_worker_st *worker,
                                         const char *function_name,
                                         uint32_t timeout)
{
  return _worker_function_add(worker, function_name, timeout, NULL, NULL);
}

gearman_return_t gearman_worker_unregister(gearman_worker_st *worker,
                                           const char *function_name)
{
  gearman_worker_function_st *function;
  gearman_return_t ret;
  const void *args[1];
  size_t args_size[1];

  for (function= worker->function_list; function != NULL;
       function= function->next)
  {
    if (!strcmp(function_name, function->function_name))
      break;
  }

  if (function == NULL)
    return GEARMAN_SUCCESS;

  gearman_packet_free(&(function->packet));

  args[0]= function_name;
  args_size[0]= strlen(function_name);
  ret= gearman_add_packet_args(worker->gearman, &(function->packet),
                               GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_CANT_DO,
                               args, args_size, 1);
  if (ret != GEARMAN_SUCCESS)
  {
    function->options&=
      (gearman_worker_function_options_t)~GEARMAN_WORKER_FUNCTION_PACKET_IN_USE;
    return ret;
  }

  function->options|= (GEARMAN_WORKER_FUNCTION_CHANGE |
                       GEARMAN_WORKER_FUNCTION_REMOVE);

  worker->options|= GEARMAN_WORKER_CHANGE;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_worker_unregister_all(gearman_worker_st *worker)
{
  gearman_return_t ret;

  if (worker->function_list == NULL)
    return GEARMAN_SUCCESS;

  while (worker->function_list->next != NULL)
    _worker_function_free(worker, worker->function_list->next);

  gearman_packet_free(&(worker->function_list->packet));

  ret= gearman_add_packet_args(worker->gearman,
                               &(worker->function_list->packet),
                               GEARMAN_MAGIC_REQUEST,
                               GEARMAN_COMMAND_RESET_ABILITIES,
                               NULL, NULL, 0);
  if (ret != GEARMAN_SUCCESS)
  {
    worker->function_list->options&=
      (gearman_worker_function_options_t)~GEARMAN_WORKER_FUNCTION_PACKET_IN_USE;
    return ret;
  }

  worker->function_list->options|= (GEARMAN_WORKER_FUNCTION_CHANGE |
                                    GEARMAN_WORKER_FUNCTION_REMOVE);

  worker->options|= GEARMAN_WORKER_CHANGE;

  return GEARMAN_SUCCESS;
}

gearman_job_st *gearman_worker_grab_job(gearman_worker_st *worker,
                                        gearman_job_st *job,
                                        gearman_return_t *ret_ptr)
{
  gearman_worker_function_st *function;
  uint32_t active;

  while (1)
  {
    switch (worker->state)
    {
    case GEARMAN_WORKER_STATE_START:
      /* If there are any new functions changes, send them now. */
      if (worker->options & GEARMAN_WORKER_CHANGE)
      {
        worker->function= worker->function_list;
        while (worker->function != NULL)
        {
          if (!(worker->function->options & GEARMAN_WORKER_FUNCTION_CHANGE))
          {
            worker->function= worker->function->next;
            continue;
          }

          for (worker->con= worker->gearman->con_list; worker->con != NULL;
               worker->con= worker->con->next)
          {
            if (worker->con->fd == -1)
              continue;

    case GEARMAN_WORKER_STATE_FUNCTION_SEND:
            *ret_ptr= gearman_con_send(worker->con, &(worker->function->packet),
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

          if (worker->function->options & GEARMAN_WORKER_FUNCTION_REMOVE)
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
            worker->function->options&=
             (gearman_worker_function_options_t)~GEARMAN_WORKER_FUNCTION_CHANGE;
            worker->function= worker->function->next;
          }
        }

        worker->options&= (gearman_worker_options_t)~GEARMAN_WORKER_CHANGE;
      }

      if (worker->function_list == NULL)
      {
        gearman_set_error(worker->gearman, "gearman_worker_grab_job",
                          "no functions have been registered");
        *ret_ptr= GEARMAN_NO_REGISTERED_FUNCTIONS;
        return NULL;
      }

      for (worker->con= worker->gearman->con_list; worker->con != NULL;
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
            *ret_ptr= gearman_con_send(worker->con, &(worker->function->packet),
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

        *ret_ptr= gearman_con_send(worker->con, &(worker->grab_job), true);
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
          worker->job= _job_create(worker, job);
          if (worker->job == NULL)
          {
            *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
            return NULL;
          }
        }

        while (1)
        {
    case GEARMAN_WORKER_STATE_GRAB_JOB_RECV:
          (void)gearman_con_recv(worker->con, &(worker->job->assigned), ret_ptr,
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
            worker->job->options|= GEARMAN_JOB_ASSIGNED_IN_USE;
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
            gearman_set_error(worker->gearman, "gearman_worker_grab_job",
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
      for (worker->con= worker->gearman->con_list; worker->con != NULL;
           worker->con= worker->con->next)
      {
        if (worker->con->fd == -1)
          continue;

        *ret_ptr= gearman_con_send(worker->con, &(worker->pre_sleep), true);
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
      for (worker->con= worker->gearman->con_list; worker->con != NULL;
           worker->con= worker->con->next)
      {
        if (worker->con->fd == -1)
          continue;

        *ret_ptr= gearman_con_set_events(worker->con, POLLIN);
        if (*ret_ptr != GEARMAN_SUCCESS)
          return NULL;

        active++;
      }

      if (worker->gearman->options & GEARMAN_NON_BLOCKING)
      {
        *ret_ptr= GEARMAN_NO_JOBS;
        return NULL;
      }

      if (active == 0)
      {
        if (worker->gearman->timeout < 0)
          usleep(GEARMAN_WORKER_WAIT_TIMEOUT * 1000);
        else
        {
          if (worker->gearman->timeout > 0)
            usleep((unsigned int)worker->gearman->timeout * 1000);

          if (worker->options & GEARMAN_WORKER_TIMEOUT_RETURN)
          {
            gearman_set_error(worker->gearman, "gearman_worker_grab_job",
                              "timeout reached");
            *ret_ptr= GEARMAN_TIMEOUT;
            return NULL;
          }
        }
      }
      else
      {
        *ret_ptr= gearman_wait(worker->gearman);
        if (*ret_ptr != GEARMAN_SUCCESS && (*ret_ptr != GEARMAN_TIMEOUT ||
            worker->options & GEARMAN_WORKER_TIMEOUT_RETURN))
        {
          return NULL;
        }
      }

      break;

    default:
      gearman_set_error(worker->gearman, "gearman_worker_grab_job",
                        "unknown state: %u", worker->state);
      *ret_ptr= GEARMAN_UNKNOWN_STATE;
      return NULL;
    }
  }
}

void gearman_job_free(gearman_job_st *job)
{
  if (job->options & GEARMAN_JOB_ASSIGNED_IN_USE)
    gearman_packet_free(&(job->assigned));

  if (job->options & GEARMAN_JOB_WORK_IN_USE)
    gearman_packet_free(&(job->work));

  if (job->worker->job_list == job)
    job->worker->job_list= job->next;
  if (job->prev != NULL)
    job->prev->next= job->next;
  if (job->next != NULL)
    job->next->prev= job->prev;
  job->worker->job_count--;

  if (job->options & GEARMAN_JOB_ALLOCATED)
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
                                             const void *context)
{
  if (function_name == NULL)
  {
    gearman_set_error(worker->gearman, "gearman_worker_add_function",
                      "function name not given");
    return GEARMAN_INVALID_FUNCTION_NAME;
  }

  if (worker_fn == NULL)
  {
    gearman_set_error(worker->gearman, "gearman_worker_add_function",
                      "function not given");
    return GEARMAN_INVALID_WORKER_FUNCTION;
  }

  return _worker_function_add(worker, function_name, timeout, worker_fn,
                              context);
}

gearman_return_t gearman_worker_work(gearman_worker_st *worker)
{
  gearman_return_t ret;

  switch (worker->work_state)
  {
  case GEARMAN_WORKER_WORK_STATE_GRAB_JOB:
    (void)gearman_worker_grab_job(worker, &(worker->work_job), &ret);
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
      gearman_set_error(worker->gearman, "gearman_worker_work",
                        "function not found");
      return GEARMAN_INVALID_FUNCTION_NAME;
    }

    if (worker->work_function->worker_fn == NULL)
    {
      gearman_job_free(&(worker->work_job));
      gearman_set_error(worker->gearman, "gearman_worker_work",
                        "no callback function supplied");
      return GEARMAN_INVALID_FUNCTION_NAME;
    }

    worker->options|= GEARMAN_WORKER_WORK_JOB_IN_USE;
    worker->work_result_size= 0;

  case GEARMAN_WORKER_WORK_STATE_FUNCTION:
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

        worker->work_state= GEARMAN_WORKER_WORK_STATE_FAIL;
        return ret;
      }

      break;
    }

    if (ret != GEARMAN_SUCCESS)
    {
      if (ret == GEARMAN_LOST_CONNECTION)
        break;

      worker->work_state= GEARMAN_WORKER_WORK_STATE_FUNCTION;
      return ret;
    }

  case GEARMAN_WORKER_WORK_STATE_COMPLETE:
    ret= gearman_job_send_complete(&(worker->work_job), worker->work_result,
                              worker->work_result_size);
    if (ret == GEARMAN_IO_WAIT)
    {
      worker->work_state= GEARMAN_WORKER_WORK_STATE_COMPLETE;
      return ret;
    }

    if (worker->work_result != NULL)
    {
      if (worker->gearman->workload_free_fn == NULL)
        free(worker->work_result);
      else
      {
        worker->gearman->workload_free_fn(worker->work_result,
                                (void *)worker->gearman->workload_free_context);
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

  case GEARMAN_WORKER_WORK_STATE_FAIL:
    ret= gearman_job_send_fail(&(worker->work_job));
    if (ret != GEARMAN_SUCCESS)
    {
      if (ret == GEARMAN_LOST_CONNECTION)
        break;

      return ret;
    }

   break;

  default:
    gearman_set_error(worker->gearman, "gearman_worker_work",
                      "unknown state: %u", worker->work_state);
    return GEARMAN_UNKNOWN_STATE;
  }

  gearman_job_free(&(worker->work_job));
  worker->options&= (gearman_worker_options_t)~GEARMAN_WORKER_WORK_JOB_IN_USE;
  worker->work_state= GEARMAN_WORKER_WORK_STATE_GRAB_JOB;
  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_worker_echo(gearman_worker_st *worker,
                                     const void *workload,
                                     size_t workload_size)
{
  return gearman_echo(worker->gearman, workload, workload_size);
}

/*
 * Static Definitions
 */

static gearman_worker_st *_worker_allocate(gearman_worker_st *worker)
{
  if (worker == NULL)
  {
    worker= malloc(sizeof(gearman_worker_st));
    if (worker == NULL)
      return NULL;

    worker->options= GEARMAN_WORKER_ALLOCATED;
  }
  else
    worker->options= 0;

  worker->state= 0;
  worker->work_state= 0;
  worker->function_count= 0;
  worker->job_count= 0;
  worker->work_result_size= 0;
  worker->gearman= NULL;
  worker->con= NULL;
  worker->job= NULL;
  worker->job_list= NULL;
  worker->function= NULL;
  worker->function_list= NULL;
  worker->work_function= NULL;
  worker->work_result= NULL;

  return worker;
}

static gearman_return_t _worker_packet_init(gearman_worker_st *worker)
{
  gearman_return_t ret;

  ret= gearman_add_packet_args(worker->gearman, &(worker->grab_job),
                               GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_GRAB_JOB,
                               NULL, NULL, 0);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  ret= gearman_add_packet_args(worker->gearman, &(worker->pre_sleep),
                               GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_PRE_SLEEP,
                               NULL, NULL, 0);
  if (ret != GEARMAN_SUCCESS)
  {
    gearman_packet_free(&(worker->grab_job));
    return ret;
  }

  worker->options|= GEARMAN_WORKER_PACKET_INIT;

  return GEARMAN_SUCCESS;
}

static gearman_return_t _worker_add_server(const char *host, in_port_t port,
                                           void *context)
{
  return gearman_worker_add_server((gearman_worker_st *)context, host, port);
}

static gearman_return_t _worker_function_add(gearman_worker_st *worker,
                                             const char *function_name,
                                             uint32_t timeout,
                                             gearman_worker_fn *worker_fn,
                                             const void *context)
{
  gearman_worker_function_st *function;
  gearman_return_t ret;
  char timeout_buffer[11];
  const void *args[2];
  size_t args_size[2];

  function= malloc(sizeof(gearman_worker_function_st));
  if (function == NULL)
  {
    gearman_set_error(worker->gearman, "_worker_function_add", "malloc");
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;
  }

  function->options= (GEARMAN_WORKER_FUNCTION_PACKET_IN_USE |
                      GEARMAN_WORKER_FUNCTION_CHANGE);

  function->function_name= strdup(function_name);
  if (function->function_name == NULL)
  {
    free(function);
    gearman_set_error(worker->gearman, "gearman_worker_add_function", "strdup");
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
    ret= gearman_add_packet_args(worker->gearman, &(function->packet),
                                 GEARMAN_MAGIC_REQUEST,
                                 GEARMAN_COMMAND_CAN_DO_TIMEOUT,
                                 args, args_size, 2);
  }
  else
  {
    args[0]= function_name;
    args_size[0]= strlen(function_name);
    ret= gearman_add_packet_args(worker->gearman, &(function->packet),
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

  worker->options|= GEARMAN_WORKER_CHANGE;

  return GEARMAN_SUCCESS;
}

static void _worker_function_free(gearman_worker_st *worker,
                                  gearman_worker_function_st *function)
{
  if (worker->function_list == function)
    worker->function_list= function->next;
  if (function->prev != NULL)
    function->prev->next= function->next;
  if (function->next != NULL)
    function->next->prev= function->prev;
  worker->function_count--;

  if (function->options & GEARMAN_WORKER_FUNCTION_PACKET_IN_USE)
    gearman_packet_free(&(function->packet));

  free(function->function_name);
  free(function);
}

static gearman_job_st *_job_create(gearman_worker_st *worker,
                                   gearman_job_st *job)
{
  if (job == NULL)
  {
    job= malloc(sizeof(gearman_job_st));
    if (job == NULL)
    {
      gearman_set_error(worker->gearman, "_job_create", "malloc");
      return NULL;
    }

    job->options= GEARMAN_JOB_ALLOCATED;
  }
  else
    job->options= 0;

  job->worker= worker;

  if (worker->job_list != NULL)
    worker->job_list->prev= job;
  job->next= worker->job_list;
  job->prev= NULL;
  worker->job_list= job;
  worker->job_count++;

  job->con= NULL;

  return job;
}
