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
#include <tests/workers.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

/* Use this for string generation */
static const char ALPHANUMERICS[]=
  "0123456789ABCDEFGHIJKLMNOPQRSTWXYZabcdefghijklmnopqrstuvwxyz";

#define ALPHANUMERICS_SIZE (sizeof(ALPHANUMERICS)-1)

static size_t get_alpha_num(void)
{
  return (size_t)random() % ALPHANUMERICS_SIZE;
}

gearman_return_t sleep_return_random_worker(gearman_job_st *job, void *)
{
  libtest::dream(WORKER_DEFAULT_SLEEP, 0);

  char buffer[1024];
  for (size_t x= 0; x < sizeof(buffer); x++)
  {
    buffer[x]= ALPHANUMERICS[get_alpha_num()];
  }
  buffer[sizeof(buffer) -1]= 0;

  if (gearman_failed(gearman_job_send_data(job, buffer, sizeof(buffer))))
  {
    return GEARMAN_ERROR;
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t echo_or_react_worker_v2(gearman_job_st *job, void *)
{
  const void *workload= gearman_job_workload(job);
  size_t result_size= gearman_job_workload_size(job);

  if (workload == NULL or result_size == 0)
  {
    assert(workload == NULL and result_size == 0);
    return GEARMAN_SUCCESS;
  }
  else if (result_size == test_literal_param_size("fail") and (not memcmp(workload, test_literal_param("fail"))))
  {
    return GEARMAN_FAIL;
  }
  else if (result_size == test_literal_param_size("sleep") and (not memcmp(workload, test_literal_param("sleep"))))
  {
    libtest::dream(WORKER_DEFAULT_SLEEP, 0);
    if (gearman_failed(gearman_job_send_data(job, test_literal_param("slept"))))
    {
      return GEARMAN_ERROR;
    }
    return GEARMAN_SUCCESS;
  }
  else if (result_size == test_literal_param_size("exception") and (not memcmp(workload, test_literal_param("exception"))))
  {
    gearman_return_t rc= gearman_job_send_exception(job, test_literal_param("test exception"));
    if (gearman_failed(rc))
    {
      return GEARMAN_ERROR;
    }
  }
  else if (result_size == test_literal_param_size("warning") and (not memcmp(workload, test_literal_param("warning"))))
  {
    gearman_return_t rc= gearman_job_send_warning(job, test_literal_param("test warning"));
    if (gearman_failed(rc))
    {
      return GEARMAN_ERROR;
    }
  }

  if (gearman_failed(gearman_job_send_data(job, workload, result_size)))
  {
    return GEARMAN_ERROR;
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t echo_or_react_chunk_worker_v2(gearman_job_st *job, void *split_arg)
{
  const char *workload= (const char *)gearman_job_workload(job);
  size_t workload_size= gearman_job_workload_size(job);

  bool fail= false;
  if (workload_size == test_literal_param_size("fail") and (not memcmp(workload, test_literal_param("fail"))))
  {
    fail= true;
  }
  else if (workload_size == test_literal_param_size("exception") and (not memcmp(workload, test_literal_param("exception"))))
  {
    if (gearman_failed(gearman_job_send_exception(job, test_literal_param("test exception"))))
    {
      return GEARMAN_ERROR;
    }
  }
  else if (workload_size == test_literal_param_size("warning") and (not memcmp(workload, test_literal_param("warning"))))
  {
    if (gearman_failed(gearman_job_send_warning(job, test_literal_param("test warning"))))
    {
      return GEARMAN_ERROR;
    }
  }

  size_t split_on= 1; 
  if (split_arg)
  {
    size_t *tmp= (size_t*)split_arg;
    split_on= *tmp;
  }

  if (split_on > workload_size)
  {
    split_on= workload_size;
  }

  const char* workload_ptr= &workload[0];
  size_t remaining= workload_size;
  for (size_t x= 0; x < workload_size; x+= split_on)
  {
    // Chunk
    if (gearman_failed(gearman_job_send_data(job, workload_ptr, split_on)))
    {
      return GEARMAN_ERROR;
    }
    remaining-= split_on;
    workload_ptr+= split_on;

    // report status
    {
      if (gearman_failed(gearman_job_send_status(job, (uint32_t)x, (uint32_t)workload_size)))
      {
        return GEARMAN_ERROR;
      }

      if (fail)
      {
        return GEARMAN_FAIL;
      }
    }
  }

  if (remaining)
  {
    if (gearman_failed(gearman_job_send_data(job, workload_ptr, remaining)))
    {
      return GEARMAN_ERROR;
    }
  }

  return GEARMAN_SUCCESS;
}

// payload is unique value
gearman_return_t unique_worker_v2(gearman_job_st *job, void *)
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
      if (gearman_failed(gearman_job_send_data(job, workload, gearman_job_workload_size(job))))
      {
        return GEARMAN_ERROR;
      }

      return GEARMAN_SUCCESS;
    }
  }

  return GEARMAN_FAIL;
}

gearman_return_t count_worker(gearman_job_st *job, void *)
{
  char buffer[GEARMAN_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];

  int length= snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(gearman_job_workload_size(job)));

  if (size_t(length) > sizeof(buffer) or length < 0)
  {
    return GEARMAN_FAIL;
  }

  return GEARMAN_SUCCESS;
}

static pthread_mutex_t increment_reset_worker_mutex= PTHREAD_MUTEX_INITIALIZER;

gearman_return_t increment_reset_worker_v2(gearman_job_st *job, void *)
{
  static long counter= 0;
  long change= 0;
  const char *workload= (const char*)gearman_job_workload(job);

  if (gearman_job_workload_size(job) == test_literal_param_size("reset") and (not memcmp(workload, test_literal_param("reset"))))
  {
    pthread_mutex_lock(&increment_reset_worker_mutex);
    counter= 0;
    pthread_mutex_unlock(&increment_reset_worker_mutex);

    return GEARMAN_SUCCESS;
  }
  else if (workload and gearman_job_workload_size(job))
  {
    if (gearman_job_workload_size(job) > GEARMAN_MAXIMUM_INTEGER_DISPLAY_LENGTH)
    {
      return GEARMAN_FAIL;
    }

    char temp[GEARMAN_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
    memcpy(temp, workload, gearman_job_workload_size(job));
    temp[gearman_job_workload_size(job)]= 0;
    change= strtol(temp, (char **)NULL, 10);
    if (change ==  LONG_MIN or change == LONG_MAX or ( change == 0 and errno < 0))
    {
      gearman_job_send_warning(job, test_literal_param("strtol() failed"));
      return GEARMAN_FAIL;
    }
  }

  {
    pthread_mutex_lock(&increment_reset_worker_mutex);
    counter= counter +change;

    char result[GEARMAN_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
    size_t result_size= size_t(snprintf(result, sizeof(result), "%ld", counter));
    if (gearman_failed(gearman_job_send_data(job, result, result_size)))
    {
      pthread_mutex_unlock(&increment_reset_worker_mutex);
      return GEARMAN_FAIL;
    }

    pthread_mutex_unlock(&increment_reset_worker_mutex);
  }

  return GEARMAN_SUCCESS;
}
