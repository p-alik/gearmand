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


/**
 * @file
 * @brief Job Definitions
 */

#include <libgearman/common.h>
#include <libgearman/connection.h>
#include <libgearman/packet.h>
#include <cstdio>
#include <cstring>
#include <memory>

/**
 * @addtogroup gearman_job_static Static Job Declarations
 * @ingroup gearman_job
 * @{
 */

/**
 * Send a packet for a job.
 */
static gearman_return_t _job_send(gearman_job_st *job);

/** @} */

/*
 * Public Definitions
 */

gearman_job_st *gearman_job_create(gearman_worker_st *worker, gearman_job_st *job)
{
  if (not job)
  {
    job= new (std::nothrow) gearman_job_st;
    if (not job)
    {
      gearman_universal_set_error((&worker->universal),
                                  GEARMAN_MEMORY_ALLOCATION_FAILURE,
                                  "_job_create", "malloc");
      return NULL;
    }

    job->options.allocated= true;
  }
  else
  {
    job->options.allocated= false;
  }

  job->options.assigned_in_use= false;
  job->options.work_in_use= false;
  job->options.finished= false;

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


gearman_return_t gearman_job_send_data(gearman_job_st *job, const void *data,
                                       size_t data_size)
{
  gearman_return_t ret;
  const void *args[2];
  size_t args_size[2];

#if 0
  if (job->each_fn)
  {
    gearman_argument_t value1= gearman_argument_make(data, data_size);
    gearman_task_add_work(task, &value1), gearman_client_error(client);

    return GEARMAN_SUCCESS;
  }
#endif

  if (not (job->options.work_in_use))
  {
    args[0]= job->assigned.arg[0];
    args_size[0]= job->assigned.arg_size[0];
    args[1]= data;
    args_size[1]= data_size;
    ret= gearman_packet_create_args(&(job->worker->universal), &(job->work),
                                    GEARMAN_MAGIC_REQUEST,
                                    GEARMAN_COMMAND_WORK_DATA,
                                    args, args_size, 2);
    if (gearman_failed(ret))
      return ret;

    job->options.work_in_use= true;
  }

  return _job_send(job);
}

gearman_return_t gearman_job_send_warning(gearman_job_st *job,
                                          const void *warning,
                                          size_t warning_size)
{
  gearman_return_t ret;
  const void *args[2];
  size_t args_size[2];

  if (not (job->options.work_in_use))
  {
    args[0]= job->assigned.arg[0];
    args_size[0]= job->assigned.arg_size[0];
    args[1]= warning;
    args_size[1]= warning_size;
    ret= gearman_packet_create_args(&(job->worker->universal), &(job->work),
                                    GEARMAN_MAGIC_REQUEST,
                                    GEARMAN_COMMAND_WORK_WARNING,
                                    args, args_size, 2);
    if (gearman_failed(ret))
      return ret;

    job->options.work_in_use= true;
  }

  return _job_send(job);
}

gearman_return_t gearman_job_send_status(gearman_job_st *job,
                                         uint32_t numerator,
                                         uint32_t denominator)
{
  gearman_return_t ret;
  char numerator_string[12];
  char denominator_string[12];
  const void *args[3];
  size_t args_size[3];

  if (not (job->options.work_in_use))
  {
    snprintf(numerator_string, 12, "%u", numerator);
    snprintf(denominator_string, 12, "%u", denominator);

    args[0]= job->assigned.arg[0];
    args_size[0]= job->assigned.arg_size[0];
    args[1]= numerator_string;
    args_size[1]= strlen(numerator_string) + 1;
    args[2]= denominator_string;
    args_size[2]= strlen(denominator_string);
    ret= gearman_packet_create_args(&(job->worker->universal), &(job->work),
                                    GEARMAN_MAGIC_REQUEST,
                                    GEARMAN_COMMAND_WORK_STATUS,
                                    args, args_size, 3);
    if (gearman_failed(ret))
      return ret;

    job->options.work_in_use= true;
  }

  return _job_send(job);
}

gearman_return_t gearman_job_send_complete(gearman_job_st *job,
                                           const void *result,
                                           size_t result_size)
{
  gearman_return_t ret;
  const void *args[2];
  size_t args_size[2];

  if (job->options.finished)
    return GEARMAN_SUCCESS;

  if (not (job->options.work_in_use))
  {
    args[0]= job->assigned.arg[0];
    args_size[0]= job->assigned.arg_size[0];
    args[1]= result;
    args_size[1]= result_size;
    ret= gearman_packet_create_args(&(job->worker->universal), &(job->work),
                                 GEARMAN_MAGIC_REQUEST,
                                 GEARMAN_COMMAND_WORK_COMPLETE,
                                 args, args_size, 2);
    if (gearman_failed(ret))
      return ret;

    job->options.work_in_use= true;
  }

  ret= _job_send(job);
  if (gearman_failed(ret))
    return ret;

  job->options.finished= true;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_job_send_exception(gearman_job_st *job,
                                            const void *exception,
                                            size_t exception_size)
{
  const void *args[2];
  size_t args_size[2];

  if (not (job->options.work_in_use))
  {
    args[0]= job->assigned.arg[0];
    args_size[0]= job->assigned.arg_size[0];
    args[1]= exception;
    args_size[1]= exception_size;

    gearman_return_t ret;
    ret= gearman_packet_create_args(&(job->worker->universal), &(job->work),
                                    GEARMAN_MAGIC_REQUEST,
                                    GEARMAN_COMMAND_WORK_EXCEPTION,
                                    args, args_size, 2);
    if (gearman_failed(ret))
      return ret;

    job->options.work_in_use= true;
  }

  return _job_send(job);
}

gearman_return_t gearman_job_send_fail(gearman_job_st *job)
{
  const void *args[1];
  size_t args_size[1];

  if (job->options.finished)
    return GEARMAN_SUCCESS;

  if (not (job->options.work_in_use))
  {
    args[0]= job->assigned.arg[0];
    args_size[0]= job->assigned.arg_size[0] - 1;
    gearman_return_t ret;
    ret= gearman_packet_create_args(&(job->worker->universal), &(job->work),
                                    GEARMAN_MAGIC_REQUEST,
                                    GEARMAN_COMMAND_WORK_FAIL,
                                    args, args_size, 1);
    if (gearman_failed(ret))
      return ret;

    job->options.work_in_use= true;
  }

  gearman_return_t ret;
  ret= _job_send(job);
  if (gearman_failed(ret))
    return ret;

  job->options.finished= true;
  return GEARMAN_SUCCESS;
}

const char *gearman_job_handle(const gearman_job_st *job)
{
  return static_cast<const char *>(job->assigned.arg[0]);
}

const char *gearman_job_function_name(const gearman_job_st *job)
{
  return static_cast<const char *>(job->assigned.arg[1]);
}

const char *gearman_job_unique(const gearman_job_st *job)
{
  if (job->assigned.command == GEARMAN_COMMAND_JOB_ASSIGN_UNIQ)
    return static_cast<const char *>(job->assigned.arg[2]);

  return "";
}

const void *gearman_job_workload(const gearman_job_st *job)
{
  return job->assigned.data;
}

size_t gearman_job_workload_size(const gearman_job_st *job)
{
  return job->assigned.data_size;
}

void *gearman_job_take_workload(gearman_job_st *job, size_t *data_size)
{
  return gearman_packet_take_data(&(job->assigned), data_size);
}

/*
 * Static Definitions
 */

static gearman_return_t _job_send(gearman_job_st *job)
{
  gearman_return_t ret;

  ret= gearman_connection_send(job->con, &(job->work), true);
  if (gearman_failed(ret))
    return ret;

  gearman_packet_free(&(job->work));
  job->options.work_in_use= false;

  return GEARMAN_SUCCESS;
}
