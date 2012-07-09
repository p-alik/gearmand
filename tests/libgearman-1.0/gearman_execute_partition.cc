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

#include <cassert>
#include <cstring>
#include <libgearman/gearman.h>
#include "tests/gearman_execute_partition.h"

#include "tests/libgearman-1.0/client_test.h"

#include "tests/workers.h"

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#define WORKER_FUNCTION_NAME "client_test"
#define WORKER_SPLIT_FUNCTION_NAME "split_worker"

namespace {
  gearman_return_t cat_aggregator_fn(gearman_aggregator_st *, gearman_task_st *task, gearman_result_st *result)
  {
    std::string string_value;

    do
    {
      assert(task);
      gearman_result_st *result_ptr= gearman_task_result(task);

      if (result_ptr)
      {
        if (gearman_result_size(result_ptr) == 0)
        {
          return GEARMAN_WORK_EXCEPTION;
        }

        string_value.append(gearman_result_value(result_ptr), gearman_result_size(result_ptr));
      }
    } while ((task= gearman_next(task)));

    gearman_result_store_value(result, string_value.c_str(), string_value.size());

    return GEARMAN_SUCCESS;
  }

  gearman_return_t split_worker(gearman_job_st *job, void* /* context */)
  {
    const char *workload= static_cast<const char *>(gearman_job_workload(job));
    size_t workload_size= gearman_job_workload_size(job);

    assert(job->assigned.command == GEARMAN_COMMAND_JOB_ASSIGN_ALL);

    const char *chunk_begin= workload;
    for (size_t x= 0; x < workload_size; x++)
    {
      if (int(workload[x]) == 0 or int(workload[x]) == int(' '))
      {
        if ((workload +x -chunk_begin) == 11 and not memcmp(chunk_begin, test_literal_param("mapper_fail")))
        {
          return GEARMAN_FAIL;
        }

        // NULL Chunk
        gearman_return_t rc= gearman_job_send_data(job, chunk_begin, workload +x -chunk_begin);
        if (gearman_failed(rc))
        {
          return GEARMAN_FAIL;
        }

        chunk_begin= workload +x +1;
      }
    }

    if (chunk_begin < workload +workload_size)
    {
      if ((size_t(workload +workload_size) -size_t(chunk_begin) ) == 11 and not memcmp(chunk_begin, test_literal_param("mapper_fail")))
      {
        return GEARMAN_FAIL;
      }

      gearman_return_t rc= gearman_job_send_data(job, chunk_begin, size_t(workload +workload_size) -size_t(chunk_begin));
      if (gearman_failed(rc))
      {
        return GEARMAN_FAIL;
      }
    }

    return GEARMAN_SUCCESS;
  }
}

test_return_t partition_SETUP(void *object)
{
  client_test_st *test= (client_test_st *)object;
  test_true(test);

  test->set_worker_name(WORKER_FUNCTION_NAME);

  gearman_function_t echo_react_fn= gearman_function_create_v2(echo_or_react_worker_v2);
  test->push(test_worker_start(libtest::default_port(), NULL,
                               test->worker_name(),
                               echo_react_fn, NULL, gearman_worker_options_t()));

  gearman_function_t split_worker_fn= gearman_function_create_partition(split_worker, cat_aggregator_fn);
  test->push(test_worker_start(libtest::default_port(), NULL,
                               WORKER_SPLIT_FUNCTION_NAME,
                               split_worker_fn,  NULL, GEARMAN_WORKER_GRAB_ALL));


  return TEST_SUCCESS;
}

test_return_t partition_free_SETUP(void *object)
{
  client_test_st *test= (client_test_st *)object;

  test_compare(TEST_SUCCESS, partition_SETUP(object));

  gearman_client_add_options(test->client(), GEARMAN_CLIENT_FREE_TASKS);

  return TEST_SUCCESS;
}

test_return_t gearman_execute_partition_check_parameters(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  test_true(client);

  test_compare(GEARMAN_SUCCESS,
               gearman_client_echo(client, test_literal_param("this is mine")));

  // This just hear to make it easier to trace when gearman_execute() is
  // called (look in the log to see the failed option setting.
  gearman_client_set_server_option(client, test_literal_param("should fail"));
  gearman_argument_t workload= gearman_argument_make(0, 0, test_literal_param("this dog does not hunt"));

  // Test client as NULL
  gearman_task_st *task= gearman_execute_by_partition(NULL,
                                                      test_literal_param("split_worker"),
                                                      test_literal_param(WORKER_FUNCTION_NAME),
                                                      NULL, 0,  // unique
                                                      NULL,
                                                      &workload, 0);
  test_null(task);

  // Test no partition function
  task= gearman_execute_by_partition(client,
                                     NULL, 0,
                                     NULL, 0,
                                     NULL, 0,  // unique
                                     NULL,
                                     &workload, 0);
  test_null(task);

  return TEST_SUCCESS;
}

test_return_t gearman_execute_partition_basic(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  test_compare(GEARMAN_SUCCESS,
               gearman_client_echo(client, test_literal_param("this is mine")));

  // This just hear to make it easier to trace when
  // gearman_execute_partition() is called (look in the log to see the
  // failed option setting.
  gearman_client_set_server_option(client, test_literal_param("should fail"));
  gearman_argument_t workload= gearman_argument_make(0, 0, test_literal_param("this dog does not hunt"));

  gearman_task_st *task= gearman_execute_by_partition(client,
                                                      test_literal_param("split_worker"),
                                                      test_literal_param(WORKER_FUNCTION_NAME),
                                                      NULL, 0,  // unique
                                                      NULL,
                                                      &workload, 0);
  test_true(task);
  test_compare(GEARMAN_SUCCESS, gearman_task_return(task));
  gearman_result_st *result= gearman_task_result(task);
  test_truth(result);
  const char *value= gearman_result_value(result);
  test_truth(value);
  test_compare(18UL, gearman_result_size(result));

  gearman_task_free(task);
  gearman_client_task_free_all(client);

  return TEST_SUCCESS;
}

test_return_t gearman_execute_partition_workfail(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);

  test_compare(GEARMAN_SUCCESS,
               gearman_client_echo(client, test_literal_param("this is mine")));

  gearman_argument_t workload= gearman_argument_make(0, 0, test_literal_param("this dog does not hunt mapper_fail"));

  gearman_task_st *task= gearman_execute_by_partition(client,
                                                      test_literal_param("split_worker"),
                                                      gearman_string_param_cstr(worker_function),
                                                      NULL, 0,  // unique
                                                      NULL,
                                                      &workload, 0);
  test_true(task);

  test_compare(GEARMAN_WORK_FAIL, gearman_task_return(task));

  gearman_task_free(task);
  gearman_client_task_free_all(client);

  return TEST_SUCCESS;
}

test_return_t gearman_execute_partition_fail_in_reduction(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);

  test_compare(gearman_client_echo(client, test_literal_param("this is mine")), GEARMAN_SUCCESS);

  gearman_argument_t workload= gearman_argument_make(0, 0, test_literal_param("this dog does not hunt fail"));

  gearman_task_st *task= gearman_execute_by_partition(client,
                                                      test_literal_param("split_worker"),
                                                      gearman_string_param_cstr(worker_function),
                                                      NULL, 0,  // unique
                                                      NULL,
                                                      &workload, 0);
  test_true(task);

  test_compare(GEARMAN_WORK_FAIL, gearman_task_return(task));

  gearman_task_free(task);
  gearman_client_task_free_all(client);

  return TEST_SUCCESS;
}

test_return_t gearman_execute_partition_use_as_function(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  test_compare(gearman_client_echo(client, test_literal_param("this is mine")), GEARMAN_SUCCESS);

  // This just hear to make it easier to trace when
  // gearman_execute_partition() is called (look in the log to see the
  // failed option setting.
  gearman_client_set_server_option(client, test_literal_param("should fail"));
  gearman_argument_t workload= gearman_argument_make(0, 0, test_literal_param("this dog does not hunt"));

  gearman_task_st *task= gearman_execute(client,
                                         test_literal_param("split_worker"),
                                         NULL, 0,  // unique
                                         NULL,
                                         &workload, 0);
  test_true(task);

  test_compare(GEARMAN_SUCCESS, gearman_task_return(task));
  gearman_result_st *result= gearman_task_result(task);
  test_truth(result);
  const char *value= gearman_result_value(result);
  test_truth(value);
  test_compare(18UL, gearman_result_size(result));

  gearman_task_free(task);
  gearman_client_task_free_all(client);

  return TEST_SUCCESS;
}

test_return_t gearman_execute_partition_no_aggregate(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  gearman_argument_t workload= gearman_argument_make(0, 0, test_literal_param("this dog does not hunt"));

  gearman_task_st *task= gearman_execute_by_partition(client,
                                                      test_literal_param(WORKER_FUNCTION_NAME),
                                                      test_literal_param("count"),
                                                      NULL, 0,  // unique
                                                      NULL,
                                                      &workload, 0);
  test_true(task);

  test_compare(GEARMAN_SUCCESS, 
               gearman_task_return(task));

  test_compare(GEARMAN_SUCCESS, gearman_task_return(task));
  test_false(gearman_task_result(task));

  gearman_task_free(task);
  gearman_client_task_free_all(client);

  return TEST_SUCCESS;
}
