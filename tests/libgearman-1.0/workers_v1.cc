/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
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

#include <config.h>
#include <libtest/test.hpp>

using namespace libtest;

#include <libgearman/gearman.h>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cstdio>
#include <string>
#include <iostream>
#include <tests/workers_v1.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

void *echo_or_react_worker(gearman_job_st *job, void *,
                           size_t *result_size, gearman_return_t *ret_ptr)
{
  const void *workload= gearman_job_workload(job);
  *result_size= gearman_job_workload_size(job);

  if (workload == NULL or *result_size == 0)
  {
    assert(workload == NULL and *result_size == 0);
    *ret_ptr= GEARMAN_SUCCESS;
    return NULL;
  }
  else if (*result_size == test_literal_param_size("fail") and (not memcmp(workload, test_literal_param("fail"))))
  {
    *ret_ptr= GEARMAN_WORK_FAIL;
    return NULL;
  }
  else if (*result_size == test_literal_param_size("exception") and (not memcmp(workload, test_literal_param("exception"))))
  {
    gearman_return_t rc= gearman_job_send_exception(job, test_literal_param("test exception"));
    if (gearman_failed(rc))
    {
      *ret_ptr= GEARMAN_WORK_FAIL;
      return NULL;
    }
  }
  else if (*result_size == test_literal_param_size("warning") and (not memcmp(workload, test_literal_param("warning"))))
  {
    gearman_return_t rc= gearman_job_send_warning(job, test_literal_param("test warning"));
    if (gearman_failed(rc))
    {
      *ret_ptr= GEARMAN_WORK_FAIL;
      return NULL;
    }
  }

  void *result= malloc(*result_size);
  assert(result);
  memcpy(result, workload, *result_size);

  *ret_ptr= GEARMAN_SUCCESS;
  return result;
}

void *echo_or_react_chunk_worker(gearman_job_st *job, void *,
                                 size_t *result_size, gearman_return_t *ret_ptr)
{
  const char *workload;
  workload= (const char *)gearman_job_workload(job);
  size_t workload_size= gearman_job_workload_size(job);

  bool fail= false;
  if (workload_size == test_literal_param_size("fail") and (not memcmp(workload, test_literal_param("fail"))))
  {
    fail= true;
  }
  else if (workload_size == test_literal_param_size("exception") and (not memcmp(workload, test_literal_param("exception"))))
  {
    gearman_return_t rc= gearman_job_send_exception(job, test_literal_param("test exception"));
    if (gearman_failed(rc))
    {
      *ret_ptr= GEARMAN_WORK_FAIL;
      return NULL;
    }
  }
  else if (workload_size == test_literal_param_size("warning") and (not memcmp(workload, test_literal_param("warning"))))
  {
    gearman_return_t rc= gearman_job_send_warning(job, test_literal_param("test warning"));
    if (gearman_failed(rc))
    {
      *ret_ptr= GEARMAN_WORK_FAIL;
      return NULL;
    }
  }

  for (size_t x= 0; x < workload_size; x++)
  {
    // Chunk
    {
      *ret_ptr= gearman_job_send_data(job, &workload[x], 1);
      if (*ret_ptr != GEARMAN_SUCCESS)
      {
        return NULL;
      }
    }

    // report status
    {
      *ret_ptr= gearman_job_send_status(job, (uint32_t)x,
                                        (uint32_t)workload_size);
      assert(gearman_success(*ret_ptr));
      if (gearman_failed(*ret_ptr))
      {
        return NULL;
      }

      if (fail)
      {
        *ret_ptr= GEARMAN_WORK_FAIL;
        return NULL;
      }
    }
  }

  *ret_ptr= GEARMAN_SUCCESS;
  *result_size= 0;

  return NULL;
}

// payload is unique value
void *unique_worker(gearman_job_st *job, void *,
                    size_t *result_size, gearman_return_t *ret_ptr)
{
  const char *workload= static_cast<const char *>(gearman_job_workload(job));

  assert(job->assigned.command == GEARMAN_COMMAND_JOB_ASSIGN_UNIQ);
  assert(gearman_job_unique(job));
  assert(strlen(gearman_job_unique(job)));
  assert(gearman_job_workload_size(job));
  assert(strlen(gearman_job_unique(job)) == gearman_job_workload_size(job));
  assert(not memcmp(workload, gearman_job_unique(job), gearman_job_workload_size(job)));
  if (gearman_job_workload_size(job) == strlen(gearman_job_unique(job)))
  {
    if (not memcmp(workload, gearman_job_unique(job), gearman_job_workload_size(job)))
    {
      void *result= malloc(gearman_job_workload_size(job));
      assert(result);
      memcpy(result, workload, gearman_job_workload_size(job));
      *result_size= gearman_job_workload_size(job);
      *ret_ptr= GEARMAN_SUCCESS;

      return result;
    }
  }

  *result_size= 0;
  *ret_ptr= GEARMAN_WORK_FAIL;

  return NULL;
}

static pthread_mutex_t increment_reset_worker_mutex= PTHREAD_MUTEX_INITIALIZER;

void *increment_reset_worker(gearman_job_st *job, void *,
                             size_t *result_size, gearman_return_t *ret_ptr)
{
  static long counter= 0;
  long change= 0;
  const char *workload= (const char*)gearman_job_workload(job);

  if (gearman_job_workload_size(job) == test_literal_param_size("reset") and (not memcmp(workload, test_literal_param("reset"))))
  {
    pthread_mutex_lock(&increment_reset_worker_mutex);
    counter= 0;
    pthread_mutex_unlock(&increment_reset_worker_mutex);
    *ret_ptr= GEARMAN_SUCCESS;
    return NULL;
  }
  else if (workload and gearman_job_workload_size(job))
  {
    char *temp= static_cast<char *>(malloc(gearman_job_workload_size(job) +1));
    assert(temp);
    memcpy(temp, workload, gearman_job_workload_size(job));
    temp[gearman_job_workload_size(job)]= 0;
    change= strtol(temp, (char **)NULL, 10);
    free(temp);
    if (change ==  LONG_MIN or change == LONG_MAX or ( change == 0 and errno < 0))
    {
      gearman_job_send_warning(job, test_literal_param("strtol() failed"));
      *ret_ptr= GEARMAN_WORK_FAIL;
      return NULL;
    }
  }

  char *result;
  {
    pthread_mutex_lock(&increment_reset_worker_mutex);
    counter= counter +change;

    result= (char *)malloc(40);
    if (not result)
    {
      gearman_job_send_warning(job, test_literal_param("malloc() failed"));
      *ret_ptr= GEARMAN_WORK_FAIL;
      return NULL;
    }
    *result_size= size_t(snprintf(result, 40, "%ld", counter));

    pthread_mutex_unlock(&increment_reset_worker_mutex);
  }


  *ret_ptr= GEARMAN_SUCCESS;

  return result;
}
