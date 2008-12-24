/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Worker definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearman_worker_private Private Worker Functions
 * @ingroup gearman_worker
 * @{
 */

/**
 * Allocate a worker structure.
 */
static gearman_worker_st *_worker_allocate(gearman_worker_st *worker);

/** @} */

/*
 * Public definitions
 */

gearman_worker_st *gearman_worker_create(gearman_worker_st *worker)
{
  worker= _worker_allocate(worker);
  if (worker == NULL)
    return NULL;

  worker->gearman= gearman_create(&(worker->gearman_static));
  if (worker->gearman == NULL)
  { 
    gearman_worker_free(worker);
    return NULL;
  }

  return worker;
}

gearman_worker_st *gearman_worker_clone(gearman_worker_st *worker,
                                        gearman_worker_st *from)
{
  if (from == NULL)
    return NULL;

  worker= _worker_allocate(worker);
  if (worker == NULL)
    return NULL;

  worker->options|= (from->options & ~GEARMAN_WORKER_ALLOCATED);

  worker->gearman= gearman_clone(&(worker->gearman_static), from->gearman);
  if (worker->gearman == NULL)
  { 
    gearman_worker_free(worker);
    return NULL;
  }

  return worker;
}

void gearman_worker_free(gearman_worker_st *worker)
{
  gearman_free(worker->gearman);

  if (worker->options & GEARMAN_WORKER_ALLOCATED)
    free(worker);
}

void gearman_worker_reset(gearman_worker_st *worker)
{
  worker->state= GEARMAN_WORKER_STATE_GRAB_JOB;

  if (worker->job != NULL)
  {
    gearman_job_free(worker->job);
    worker->job= NULL;
  }
}

const char *gearman_worker_error(gearman_worker_st *worker)
{
  return gearman_error(worker->gearman);
}

int gearman_worker_errno(gearman_worker_st *worker)
{
  return gearman_errno(worker->gearman);
}

void gearman_worker_set_options(gearman_worker_st *worker,
                                gearman_worker_options_t options,
                                uint32_t data)
{
  if (options & GEARMAN_WORKER_NON_BLOCKING)
    gearman_set_options(worker->gearman, GEARMAN_NON_BLOCKING, data);

  if (data)
    worker->options |= options;
  else
    worker->options &= ~options;
}

gearman_return_t gearman_worker_add_server(gearman_worker_st *worker,
                                           const char *host, in_port_t port)
{
  if (gearman_con_add(worker->gearman, NULL, host, port) == NULL)
    return GEARMAN_ERRNO;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_worker_register(gearman_worker_st *worker,
                                         const char *function_name,
                                         uint32_t timeout)
{
  gearman_return_t ret;
  char timeout_buffer[11];

  if (!(worker->options & GEARMAN_WORKER_PACKET_IN_USE))
  {
    if (timeout > 0)
    {
      snprintf(timeout_buffer, 11, "%u", timeout);
      ret= gearman_packet_add(worker->gearman, &(worker->packet),
                              GEARMAN_MAGIC_REQUEST,
                              GEARMAN_COMMAND_CAN_DO_TIMEOUT,
                              (uint8_t *)function_name,
                              strlen(function_name) + 1,
                              (uint8_t *)timeout_buffer,
                              strlen(timeout_buffer), NULL);
    }
    else
    {
      ret= gearman_packet_add(worker->gearman, &(worker->packet),
                              GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_CAN_DO,
                              (uint8_t *)function_name, strlen(function_name),
                              NULL);
    }
    if (ret != GEARMAN_SUCCESS)
      return ret;

    worker->options|= GEARMAN_WORKER_PACKET_IN_USE;
  }

  ret= gearman_con_send_all(worker->gearman, &(worker->packet));
  if (ret != GEARMAN_IO_WAIT)
  {
    gearman_packet_free(&(worker->packet));
    worker->options&= ~GEARMAN_WORKER_PACKET_IN_USE;
  }

  return ret;
}

/* Unregister function with job servers. */
gearman_return_t gearman_worker_unregister(gearman_worker_st *worker,
                                           const char *function_name)
{
  gearman_return_t ret;

  if (!(worker->options & GEARMAN_WORKER_PACKET_IN_USE))
  {
    ret= gearman_packet_add(worker->gearman, &(worker->packet),
                            GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_CANT_DO,
                            (uint8_t *)function_name, strlen(function_name),
                            NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    worker->options|= GEARMAN_WORKER_PACKET_IN_USE;
  }

  ret= gearman_con_send_all(worker->gearman, &(worker->packet));
  if (ret != GEARMAN_IO_WAIT)
  {
    gearman_packet_free(&(worker->packet));
    worker->options&= ~GEARMAN_WORKER_PACKET_IN_USE;
  }

  return ret;
}

/* Unregister all functions with job servers. */
gearman_return_t gearman_worker_unregister_all(gearman_worker_st *worker)
{
  gearman_return_t ret;

  if (!(worker->options & GEARMAN_WORKER_PACKET_IN_USE))
  {
    ret= gearman_packet_add(worker->gearman, &(worker->packet),
                            GEARMAN_MAGIC_REQUEST,
                            GEARMAN_COMMAND_RESET_ABILITIES, NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    worker->options|= GEARMAN_WORKER_PACKET_IN_USE;
  }

  ret= gearman_con_send_all(worker->gearman, &(worker->packet));
  if (ret != GEARMAN_IO_WAIT)
  {
    gearman_packet_free(&(worker->packet));
    worker->options&= ~GEARMAN_WORKER_PACKET_IN_USE;
  }

  return ret;
}

gearman_job_st *gearman_worker_grab_job(gearman_worker_st *worker,
                                        gearman_job_st *job,
                                        gearman_return_t *ret)
{
  if (worker->job == NULL)
  {
    worker->job= gearman_job_create(worker->gearman, job);
    if (worker->job == NULL)
    {
      *ret= GEARMAN_ERRNO;
      return NULL;
    }
  }

  while (1)
  {
    switch (worker->state)
    {
    case GEARMAN_WORKER_STATE_INIT:
      *ret= gearman_packet_add(worker->gearman, &(worker->grab_job),
                               GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_GRAB_JOB,
                               NULL);
      if (*ret != GEARMAN_SUCCESS)
        return NULL;

      *ret= gearman_packet_add(worker->gearman, &(worker->pre_sleep),
                               GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_PRE_SLEEP,
                               NULL);
      if (*ret != GEARMAN_SUCCESS)
        return NULL;

    case GEARMAN_WORKER_STATE_GRAB_JOB:
      worker->con= worker->gearman->con_list;

      while (worker->con != NULL)
      {
    case GEARMAN_WORKER_STATE_GRAB_JOB_SEND:
        *ret= gearman_con_send(worker->con, &(worker->grab_job), true);
        if (*ret != GEARMAN_SUCCESS)
        {
          if (*ret == GEARMAN_IO_WAIT)
            worker->state= GEARMAN_WORKER_STATE_GRAB_JOB_SEND;

          return NULL;
        }

    case GEARMAN_WORKER_STATE_GRAB_JOB_RECV:
        while (1)
        {
          (void)gearman_con_recv(worker->con, &(worker->job->assigned), ret,
                                 true);
          if (*ret != GEARMAN_SUCCESS)
          {
            if (*ret == GEARMAN_IO_WAIT)
              worker->state= GEARMAN_WORKER_STATE_GRAB_JOB_RECV;

            return NULL;
          }

          if (worker->job->assigned.command == GEARMAN_COMMAND_JOB_ASSIGN)
          {
            worker->job->con= worker->con;
            worker->state= GEARMAN_WORKER_STATE_GRAB_JOB_SEND;
            job= worker->job;
            worker->job= NULL;
            return job;
          }

          if (worker->job->assigned.command != GEARMAN_COMMAND_NOOP)
          {
            gearman_packet_free(&(worker->job->assigned));
            break;
          }

          gearman_packet_free(&(worker->job->assigned));
        }

    case GEARMAN_WORKER_STATE_GRAB_JOB_NEXT:
        worker->con= worker->con->next;
      }

    case GEARMAN_WORKER_STATE_PRE_SLEEP:
      *ret= gearman_con_send_all(worker->gearman, &(worker->pre_sleep));
      if (*ret != GEARMAN_SUCCESS)
      {
        if (*ret == GEARMAN_IO_WAIT)
          worker->state= GEARMAN_WORKER_STATE_PRE_SLEEP;

        return NULL;
      }

      worker->state= GEARMAN_WORKER_STATE_GRAB_JOB;

      if (worker->gearman->options & GEARMAN_NON_BLOCKING)
      {
        *ret= GEARMAN_IO_WAIT;
        return NULL;
      }

      *ret= gearman_con_wait(worker->gearman, true);
      if (*ret != GEARMAN_SUCCESS)
        return NULL;
    }
  }

  return NULL;
}

gearman_return_t gearman_worker_add_function(gearman_worker_st *worker,
                                             const char *function_name,
                                             uint32_t timeout,
                                             gearman_worker_fn *worker_fn,
                                             const void *fn_arg)
{
  gearman_worker_function_st *function_list;

  if (!(worker->options & GEARMAN_WORKER_PACKET_IN_USE))
  {
    if (function_name == NULL)
      return GEARMAN_INVALID_FUNCTION_NAME;

    if (worker_fn == NULL)
      return GEARMAN_INVALID_WORKER_FUNCTION;

    function_list= realloc(worker->function_list,
                           sizeof(gearman_worker_function_st) *
                           worker->function_count + 1);
    if (function_list == NULL)
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;

    worker->function_list= function_list;

    function_list[worker->function_count].function_name= strdup(function_name);
    if (function_list[worker->function_count].function_name == NULL)
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;

    function_list[worker->function_count].worker_fn= worker_fn;
    function_list[worker->function_count].fn_arg= fn_arg;
    worker->function_count++;
  }

  return gearman_worker_register(worker, function_name, timeout);
}

gearman_return_t gearman_worker_work(gearman_worker_st *worker)
{
  gearman_return_t ret;
  uint32_t x;

  switch (worker->work_state)
  {
  case GEARMAN_WORKER_WORK_STATE_GRAB_JOB:
    (void)gearman_worker_grab_job(worker, &(worker->work_job), &ret);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    for (x= 0; x < worker->function_count; x++)
    {
      if (!strcmp(gearman_job_function_name(&(worker->work_job)),
                  worker->function_list[x].function_name))
      {
        worker->work_function= &(worker->function_list[x]);
        break;
      }
    }

    if (x == worker->function_count)
    {
      gearman_job_free(&(worker->work_job));
      return GEARMAN_INVALID_FUNCTION_NAME;
    }

  case GEARMAN_WORKER_WORK_STATE_FUNCTION:
    worker->work_result= (*(worker->work_function->worker_fn))(
                         &(worker->work_job),
                         (void *)(worker->work_function->fn_arg),
                         &(worker->work_result_size), &ret);
    if (ret == GEARMAN_WORK_FAIL)
    {
      ret= gearman_job_fail(&(worker->work_job));
      if (ret != GEARMAN_SUCCESS)
      {
        worker->work_state= GEARMAN_WORKER_WORK_STATE_FAIL;
        return ret;
      }

      break;
    }

    if (ret != GEARMAN_SUCCESS)
    {
      worker->work_state= GEARMAN_WORKER_WORK_STATE_FUNCTION;
      return ret;
    }

  case GEARMAN_WORKER_WORK_STATE_COMPLETE:
    ret= gearman_job_complete(&(worker->work_job), worker->work_result,
                              worker->work_result_size);
    if (ret != GEARMAN_SUCCESS)
    {
      worker->work_state= GEARMAN_WORKER_WORK_STATE_COMPLETE;
      return ret;
    }

    if (worker->work_result != NULL)
      free(worker->work_result);

    break;

  case GEARMAN_WORKER_WORK_STATE_FAIL:
    ret= gearman_job_fail(&(worker->work_job));
    if (ret != GEARMAN_SUCCESS)
      return ret;
  }

  gearman_job_free(&(worker->work_job));
  worker->work_state= GEARMAN_WORKER_WORK_STATE_GRAB_JOB;
  return GEARMAN_SUCCESS;
}

/*
 * Private definitions
 */

static gearman_worker_st *_worker_allocate(gearman_worker_st *worker)
{
  if (worker == NULL)
  {
    worker= malloc(sizeof(gearman_worker_st));
    if (worker == NULL)
      return NULL;

    memset(worker, 0, sizeof(gearman_worker_st));
    worker->options|= GEARMAN_WORKER_ALLOCATED;
  }
  else
    memset(worker, 0, sizeof(gearman_worker_st));

  return worker;
}
