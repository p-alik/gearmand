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

#include "common.h"

/* Initialize a worker structure. */
gearman_worker_st *gearman_worker_create(gearman_worker_st *worker)
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

  (void)gearman_create(&(worker->gearman));

  return worker;
}

/* Free a worker structure. */
void gearman_worker_free(gearman_worker_st *worker)
{
  if (worker->options & GEARMAN_WORKER_ALLOCATED)
    free(worker);
}

/* Return an error string for last library error encountered. */
char *gearman_worker_error(gearman_worker_st *worker)
{
  return gearman_error(&(worker->gearman));
}

/* Value of errno in the case of a GEARMAN_ERRNO return value. */
int gearman_worker_errno(gearman_worker_st *worker)
{
  return gearman_errno(&(worker->gearman));
}

/* Set options for a library instance structure. */
void gearman_worker_set_options(gearman_worker_st *worker,
                                gearman_options options,
                                uint32_t data)
{
  gearman_set_options(&(worker->gearman), options, data);
}

/* Add a job server to a worker. */
gearman_return gearman_worker_server_add(gearman_worker_st *worker, char *host,
                                         in_port_t port)
{
  if (gearman_con_add(&(worker->gearman), NULL, host, port) == NULL)
    return GEARMAN_ERRNO;

  return GEARMAN_SUCCESS;
}

/* Register function with job servers. */
gearman_return gearman_worker_register_function(gearman_worker_st *worker,
                                                char *name)
{
  gearman_return ret;

  if (!(worker->options & GEARMAN_WORKER_PACKET_IN_USE))
  {
    ret= gearman_packet_add(&(worker->gearman), &(worker->packet),
                            GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_CAN_DO,
                            name, strlen(name), NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    worker->options|= GEARMAN_WORKER_PACKET_IN_USE;
  }

  ret= gearman_con_send_all(&(worker->gearman), &(worker->packet));
  if (ret != GEARMAN_IO_WAIT)
  {
    gearman_packet_free(&(worker->packet));
    worker->options&= ~GEARMAN_WORKER_PACKET_IN_USE;
  }

  return ret;
}

/* Grab a job from one of the job servers. */
gearman_job_st *gearman_worker_grab_job(gearman_worker_st *worker,
                                        gearman_job_st *job,
                                        gearman_return *ret)
{
  if (worker->job == NULL)
  {
    worker->job= gearman_job_create(&(worker->gearman), job);
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
      *ret= gearman_packet_add(&(worker->gearman), &(worker->grab_job),
                               GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_GRAB_JOB,
                               NULL);
      if (*ret != GEARMAN_SUCCESS)
        return NULL;

      *ret= gearman_packet_add(&(worker->gearman), &(worker->pre_sleep),
                               GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_PRE_SLEEP,
                               NULL);
      if (*ret != GEARMAN_SUCCESS)
        return NULL;

    case GEARMAN_WORKER_STATE_GRAB_JOB:
      worker->con= worker->gearman.con_list;

      while (worker->con != NULL)
      {
    case GEARMAN_WORKER_STATE_GRAB_JOB_SEND:
        *ret= gearman_con_send(worker->con, &(worker->grab_job));
        if (*ret != GEARMAN_SUCCESS)
        {
          if (*ret == GEARMAN_IO_WAIT)
            worker->state= GEARMAN_WORKER_STATE_GRAB_JOB_SEND;

          return NULL;
        }

    case GEARMAN_WORKER_STATE_GRAB_JOB_RECV:
        while (1)
        {
          (void)gearman_con_recv(worker->con, &(worker->job->packet), ret);
          if (*ret != GEARMAN_SUCCESS)
          {
            if (*ret == GEARMAN_IO_WAIT)
              worker->state= GEARMAN_WORKER_STATE_GRAB_JOB_RECV;

            return NULL;
          }

          if (worker->job->packet.command == GEARMAN_COMMAND_JOB_ASSIGN)
          {
            worker->job->con= worker->con;
            worker->state= GEARMAN_WORKER_STATE_GRAB_JOB_SEND;
            job= worker->job;
            worker->job= NULL;
            return job;
          }

          if (worker->job->packet.command != GEARMAN_COMMAND_NOOP)
          {
            gearman_packet_free(&(worker->job->packet));
            break;
          }

          gearman_packet_free(&(worker->job->packet));
        }

    case GEARMAN_WORKER_STATE_GRAB_JOB_NEXT:
        worker->con= worker->con->next;
      }

    case GEARMAN_WORKER_STATE_PRE_SLEEP:
      *ret= gearman_con_send_all(&(worker->gearman), &(worker->pre_sleep));
      if (*ret != GEARMAN_SUCCESS)
      {
        if (*ret == GEARMAN_IO_WAIT)
          worker->state= GEARMAN_WORKER_STATE_PRE_SLEEP;

        return NULL;
      }

      worker->state= GEARMAN_WORKER_STATE_GRAB_JOB;

      if (worker->gearman.options & GEARMAN_NON_BLOCKING)
      {
        *ret= GEARMAN_IO_WAIT;
        return NULL;
      }

      *ret= gearman_io_wait(&(worker->gearman));
      if (*ret != GEARMAN_SUCCESS)
        return NULL;
    }
  }

  return NULL;
}
