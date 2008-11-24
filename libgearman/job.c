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
 * @brief Job definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearman_job_private Private Job Functions
 * @ingroup gearman_job
 * @{
 */

/**
 * Send a packet for a job.
 */
static gearman_return _job_send(gearman_job_st *job);

/** @} */

/*
 * Public definitions
 */

gearman_job_st *gearman_job_create(gearman_st *gearman, gearman_job_st *job)
{
  if (job == NULL)
  {
    job= malloc(sizeof(gearman_job_st));
    if (job == NULL)
      return NULL;

    memset(job, 0, sizeof(gearman_job_st));
    job->options|= GEARMAN_JOB_ALLOCATED;
  }
  else
    memset(job, 0, sizeof(gearman_job_st));

  job->gearman= gearman;

  if (gearman->job_list)
    gearman->job_list->prev= job;
  job->next= gearman->job_list;
  gearman->job_list= job;
  gearman->job_count++;

  return job;
}

void gearman_job_free(gearman_job_st *job)
{
  if (job->gearman->job_list == job)
    job->gearman->job_list= job->next;
  if (job->prev)
    job->prev->next= job->next;
  if (job->next)
    job->next->prev= job->prev;
  job->gearman->job_count--;

  if (job->options & GEARMAN_JOB_ALLOCATED)
    free(job);
}

gearman_return gearman_job_data(gearman_job_st *job, void *data,
                                size_t data_size)
{
  gearman_return ret;

  if (!(job->options & GEARMAN_JOB_WORK_IN_USE))
  {
    ret= gearman_packet_add(job->gearman, &(job->work), GEARMAN_MAGIC_REQUEST,
                            GEARMAN_COMMAND_WORK_DATA, job->assigned.arg[0],
                            job->assigned.arg_size[0], data, data_size, NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    job->options|= GEARMAN_JOB_WORK_IN_USE;
  }

  return _job_send(job);
}

gearman_return gearman_job_status(gearman_job_st *job, uint32_t numerator,
                                  uint32_t denominator)
{
  gearman_return ret;
  char numerator_string[12];
  char denominator_string[12];

  if (!(job->options & GEARMAN_JOB_WORK_IN_USE))
  {
    snprintf(numerator_string, 12, "%u", numerator);
    snprintf(denominator_string, 12, "%u", denominator);

    ret= gearman_packet_add(job->gearman, &(job->work), GEARMAN_MAGIC_REQUEST,
                            GEARMAN_COMMAND_WORK_STATUS, job->assigned.arg[0],
                            job->assigned.arg_size[0], numerator_string,
                            strlen(numerator_string) + 1, denominator_string,
                            strlen(denominator_string), NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    job->options|= GEARMAN_JOB_WORK_IN_USE;
  }

  return _job_send(job);
}

gearman_return gearman_job_complete(gearman_job_st *job, void *result,
                                    size_t result_size)
{
  gearman_return ret;

  if (!(job->options & GEARMAN_JOB_WORK_IN_USE))
  {
    ret= gearman_packet_add(job->gearman, &(job->work),
                            GEARMAN_MAGIC_REQUEST,
                            GEARMAN_COMMAND_WORK_COMPLETE,
                            job->assigned.arg[0], job->assigned.arg_size[0],
                            result, result_size, NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    job->options|= GEARMAN_JOB_WORK_IN_USE;
  }

  return _job_send(job);
}

gearman_return gearman_job_fail(gearman_job_st *job)
{
  gearman_return ret;

  if (!(job->options & GEARMAN_JOB_WORK_IN_USE))
  {
    ret= gearman_packet_add(job->gearman, &(job->work), GEARMAN_MAGIC_REQUEST,
                            GEARMAN_COMMAND_WORK_FAIL, job->assigned.arg[0],
                            job->assigned.arg_size[0], NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    job->options|= GEARMAN_JOB_WORK_IN_USE;
  }

  return _job_send(job);
}

char *gearman_job_handle(gearman_job_st *job)
{
  return (char *)job->assigned.arg[0];
}

char *gearman_job_function_name(gearman_job_st *job)
{
  return (char *)job->assigned.arg[1];
}

const void *gearman_job_workload(gearman_job_st *job)
{
  return job->assigned.data;
}

size_t gearman_job_workload_size(gearman_job_st *job)
{
  return job->assigned.data_size;
}

/*
 * Private definitions
 */

static gearman_return _job_send(gearman_job_st *job)
{
  gearman_return ret;

  ret= gearman_con_send(job->con, &(job->work), true);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  gearman_packet_free(&(job->work));
  job->options&= ~GEARMAN_JOB_WORK_IN_USE;

  return GEARMAN_SUCCESS;
}
