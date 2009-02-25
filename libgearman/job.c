/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
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
static gearman_return_t _job_send(gearman_job_st *job);

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
    {
      GEARMAN_ERROR_SET(gearman, "gearman_job_create", "malloc")
      return NULL;
    }

    memset(job, 0, sizeof(gearman_job_st));
    job->options|= GEARMAN_JOB_ALLOCATED;
  }
  else
    memset(job, 0, sizeof(gearman_job_st));

  job->gearman= gearman;

  GEARMAN_LIST_ADD(gearman->job, job,)

  return job;
}

void gearman_job_free(gearman_job_st *job)
{
  if (job->options & GEARMAN_JOB_ASSIGNED_IN_USE)
    gearman_packet_free(&(job->assigned));

  if (job->options & GEARMAN_JOB_WORK_IN_USE)
    gearman_packet_free(&(job->work));

  GEARMAN_LIST_DEL(job->gearman->job, job,)

  if (job->options & GEARMAN_JOB_ALLOCATED)
    free(job);
}

gearman_return_t gearman_job_data(gearman_job_st *job, void *data,
                                  size_t data_size)
{
  gearman_return_t ret;

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

gearman_return_t gearman_job_warning(gearman_job_st *job, void *warning,
                                     size_t warning_size)
{
  gearman_return_t ret;

  if (!(job->options & GEARMAN_JOB_WORK_IN_USE))
  {
    ret= gearman_packet_add(job->gearman, &(job->work), GEARMAN_MAGIC_REQUEST,
                            GEARMAN_COMMAND_WORK_WARNING, job->assigned.arg[0],
                            job->assigned.arg_size[0], warning, warning_size,
                            NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    job->options|= GEARMAN_JOB_WORK_IN_USE;
  }

  return _job_send(job);
}

gearman_return_t gearman_job_status(gearman_job_st *job, uint32_t numerator,
                                    uint32_t denominator)
{
  gearman_return_t ret;
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

gearman_return_t gearman_job_complete(gearman_job_st *job, void *result,
                                      size_t result_size)
{
  gearman_return_t ret;

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

gearman_return_t gearman_job_exception(gearman_job_st *job, void *exception,
                                       size_t exception_size)
{
  gearman_return_t ret;

  if (!(job->options & GEARMAN_JOB_WORK_IN_USE))
  {
    ret= gearman_packet_add(job->gearman, &(job->work), GEARMAN_MAGIC_REQUEST,
                            GEARMAN_COMMAND_WORK_EXCEPTION,
                            job->assigned.arg[0], job->assigned.arg_size[0] - 1,
                            exception, exception_size, NULL);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    job->options|= GEARMAN_JOB_WORK_IN_USE;
  }

  return _job_send(job);
}

gearman_return_t gearman_job_fail(gearman_job_st *job)
{
  gearman_return_t ret;

  if (!(job->options & GEARMAN_JOB_WORK_IN_USE))
  {
    ret= gearman_packet_add(job->gearman, &(job->work), GEARMAN_MAGIC_REQUEST,
                            GEARMAN_COMMAND_WORK_FAIL, job->assigned.arg[0],
                            job->assigned.arg_size[0] - 1, NULL);
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

char *gearman_job_unique(gearman_job_st *job)
{
  if (job->assigned.command == GEARMAN_COMMAND_JOB_ASSIGN_UNIQ)
    return (char *)job->assigned.arg[2];
  return "";
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

static gearman_return_t _job_send(gearman_job_st *job)
{
  gearman_return_t ret;

  ret= gearman_con_send(job->con, &(job->work), true);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  gearman_packet_free(&(job->work));
  job->options&= ~GEARMAN_JOB_WORK_IN_USE;

  return GEARMAN_SUCCESS;
}
