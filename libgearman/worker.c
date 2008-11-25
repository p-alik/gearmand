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

gearman_worker_st *gearman_worker_create(gearman_st *gearman,
                                         gearman_worker_st *worker)
{
  worker= _worker_allocate(worker);
  if (worker == NULL)
    return NULL;

  if (gearman == NULL)
  {
    gearman= gearman_create(&(worker->gearman_static));
    worker->options|= GEARMAN_WORKER_GEARMAN_STATIC;
  }

  worker->gearman= gearman;

  return worker;
}

gearman_worker_st *gearman_worker_clone(gearman_st *gearman,
                                        gearman_worker_st *worker,
                                        gearman_worker_st *from)
{
  worker= _worker_allocate(worker);
  if (worker == NULL)
    return NULL;

  worker->options|= (from->options & ~(GEARMAN_WORKER_ALLOCATED |
                                       GEARMAN_WORKER_GEARMAN_STATIC));

  if (gearman == NULL)
  {
    gearman= gearman_clone(&(worker->gearman_static), from->gearman);
    if (gearman == NULL)
    { 
      gearman_worker_free(worker);
      return NULL;
    }

    worker->options|= GEARMAN_WORKER_GEARMAN_STATIC;
  }

  worker->gearman= gearman;

  return worker;
}

void gearman_worker_free(gearman_worker_st *worker)
{
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
                                gearman_options options, uint32_t data)
{
  gearman_set_options(worker->gearman, options, data);
}

gearman_return gearman_worker_add_server(gearman_worker_st *worker, char *host,
                                         in_port_t port)
{
  if (gearman_con_add(worker->gearman, NULL, host, port) == NULL)
    return GEARMAN_ERRNO;

  return GEARMAN_SUCCESS;
}

gearman_return gearman_worker_register(gearman_worker_st *worker,
                                       const char *function_name,
                                       uint32_t timeout,
                                       gearman_worker_fn *worker_fn,
                                       const void *fn_arg)
{
  gearman_return ret;
  (void) timeout;

  /* Add proper error handling, and create a list or hash to support multiple
     functions. */
  assert(function_name);
  assert(worker_fn);

  if ((worker->function_name= strdup(function_name)) == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  worker->worker_fn= worker_fn;
  worker->fn_arg= fn_arg;

  if (!(worker->options & GEARMAN_WORKER_PACKET_IN_USE))
  {
    ret= gearman_packet_add(worker->gearman, &(worker->packet),
                            GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_CAN_DO,
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

#if 0
/* Unregister function with job servers. */
gearman_return gearman_worker_unregister(gearman_worker_st *worker,
                                         const char *function_name)
{
}

/* Unregister all functions with job servers. */
gearman_return gearman_worker_unregister_all(gearman_worker_st *worker)
{
}
#endif

gearman_job_st *gearman_worker_grab_job(gearman_worker_st *worker,
                                        gearman_job_st *job,
                                        gearman_return *ret)
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

gearman_return gearman_worker_work(gearman_worker_st *worker)
{
  gearman_return ret;
  gearman_job_st job;
  uint8_t *result;
  size_t result_size;

  /* This is not safe for non-blocking mode yet. */

  (void)gearman_worker_grab_job(worker, &job, &ret);
  if (ret == GEARMAN_SUCCESS)
  {
    result= (*(worker->worker_fn))(&job, (void *)worker->fn_arg,
                                   gearman_job_workload(&job),
                                   gearman_job_workload_size(&job),
                                   &result_size, &ret);
    if (ret == GEARMAN_SUCCESS)
    {
      ret= gearman_job_complete(&job, result, result_size);

      if (result_size > 0)
        free(result);
    }

    gearman_job_free(&job);
  }

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
