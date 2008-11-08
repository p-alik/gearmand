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
gearman_worker_st *gearman_worker_create(gearman_st *gearman,
                                         gearman_worker_st *worker)
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

  worker->gearman= gearman;

  if (gearman->worker_list)
    gearman->worker_list->prev= worker;
  worker->next= gearman->worker_list;
  gearman->worker_list= worker;
  gearman->worker_count++;

  return worker;
}

/* Free a worker structure. */
void gearman_worker_free(gearman_worker_st *worker)
{
  if (worker->gearman->worker_list == worker)
    worker->gearman->worker_list= worker->next;
  if (worker->prev)
    worker->prev->next= worker->next;
  if (worker->next)
    worker->next->prev= worker->prev;
  worker->gearman->worker_count--;

  if (worker->options & GEARMAN_WORKER_ALLOCATED)
    free(worker);
}

/* Register function with job servers. */
gearman_return gearman_worker_register_function(gearman_worker_st *worker,
                                                char *name)
{
  gearman_return ret;

  gearman_packet_create(worker->gearman, &(worker->packet));
  gearman_packet_set_header(&(worker->packet), GEARMAN_MAGIC_REQUEST,
                            GEARMAN_COMMAND_CAN_DO);

  ret= gearman_packet_arg_add(&(worker->packet), name, strlen(name));
  if (ret != GEARMAN_SUCCESS)
    return ret;

  ret= gearman_packet_pack(&(worker->packet));
  if (ret != GEARMAN_SUCCESS)
    return ret;

  ret= gearman_worker_send_all(worker, &(worker->packet));
  if (ret != GEARMAN_IO_WAIT)
    gearman_packet_free(&(worker->packet));

  return ret;
}

/* Send packet to all job servers. */
gearman_return gearman_worker_send_all(gearman_worker_st *worker,
                                       gearman_packet_st *packet)
{
  gearman_return ret;
  gearman_options old_options;

  if (worker->sending == 0)
  {
    old_options= worker->gearman->options;
    worker->gearman->options&= ~GEARMAN_NON_BLOCKING;

    for (worker->con= worker->gearman->con_list; worker->con != NULL;
         worker->con= worker->con->next)
    {
      ret= gearman_con_send(worker->con, packet);
      if (ret != GEARMAN_SUCCESS)
      {
        if (ret != GEARMAN_IO_WAIT)
          return ret;

        worker->sending++;
        break;
      }
    }

    worker->gearman->options= old_options;
  }

  while (worker->sending != 0)
  {
    while (1)
    {
      worker->con= gearman_io_ready(worker->gearman);
      if (worker->con == NULL)
        break;

      ret= gearman_con_send(worker->con, packet);
      if (ret != GEARMAN_SUCCESS)
      {
        if (ret != GEARMAN_IO_WAIT)
          return ret;

        continue;
      }

      worker->sending--;
    }

    if (worker->gearman->options & GEARMAN_NON_BLOCKING)
      return GEARMAN_IO_WAIT;

    ret= gearman_io_wait(worker->gearman);
    if (ret != GEARMAN_IO_WAIT)
      return ret;
  }

  return GEARMAN_SUCCESS;
}

/* Grab a job from one of the job servers. */
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
      gearman_packet_create(worker->gearman, &(worker->grab_job));
      gearman_packet_set_header(&(worker->grab_job), GEARMAN_MAGIC_REQUEST,
                                GEARMAN_COMMAND_GRAB_JOB);

      *ret= gearman_packet_pack(&(worker->grab_job));
      if (*ret != GEARMAN_SUCCESS)
        return NULL;

      gearman_packet_create(worker->gearman, &(worker->pre_sleep));
      gearman_packet_set_header(&(worker->pre_sleep), GEARMAN_MAGIC_REQUEST,
                                GEARMAN_COMMAND_PRE_SLEEP);

      *ret= gearman_packet_pack(&(worker->pre_sleep));
      if (*ret != GEARMAN_SUCCESS)
        return NULL;

    case GEARMAN_WORKER_STATE_GRAB_JOB:
      worker->con= worker->gearman->con_list;

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
      *ret= gearman_worker_send_all(worker, &(worker->pre_sleep));
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

      *ret= gearman_io_wait(worker->gearman);
      if (*ret != GEARMAN_SUCCESS)
        return NULL;
    }
  }

  return NULL;
}
