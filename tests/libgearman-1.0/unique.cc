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
#include <tests/unique.h>

#include <tests/start_worker.h>
#include "tests/workers.h"

#include "tests/client.h"

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif


test_return_t coalescence_TEST(void *object)
{
  gearman_client_st *client_one= (gearman_client_st *)object;
  test_true(client_one);

  Client client_two(client_one);

  const char* unique_handle= "local_handle";

  gearman_function_t sleep_return_random_worker_FN= gearman_function_create(sleep_return_random_worker);
  std::auto_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(),
                                                           NULL,
                                                           __func__,
                                                           sleep_return_random_worker_FN,
                                                           NULL,
                                                           gearman_worker_options_t(),
                                                           0)); // timeout

  // First task
  gearman_return_t ret;
  gearman_task_st *first_task= gearman_client_add_task(client_one,
                                                       NULL, // preallocated task
                                                       NULL, // context 
                                                       __func__, // function
                                                       unique_handle, // unique
                                                       NULL, 0, // workload
                                                       &ret);
  test_compare(GEARMAN_SUCCESS, ret);
  test_truth(first_task);
  test_true(gearman_task_unique(first_task));
  test_compare(strlen(unique_handle), strlen(gearman_task_unique(first_task)));
 
  // Second task
  gearman_task_st *second_task= gearman_client_add_task(&client_two,
                                                        NULL, // preallocated task
                                                        NULL, // context 
                                                        __func__, // function
                                                        unique_handle, // unique
                                                        NULL, 0, // workload
                                                        &ret);
  test_compare(GEARMAN_SUCCESS, ret);
  test_truth(second_task);
  test_true(gearman_task_unique(second_task));
  test_compare(strlen(unique_handle), strlen(gearman_task_unique(second_task)));
  
  test_strcmp(gearman_task_unique(first_task), gearman_task_unique(second_task));

  do {
    ret= gearman_client_run_tasks(client_one);
    gearman_client_run_tasks(&client_two);
  } while (gearman_continue(ret));

  do {
    ret= gearman_client_run_tasks(&client_two);
  } while (gearman_continue(ret));

  gearman_result_st* first_result= gearman_task_result(first_task);
  gearman_result_st* second_result= gearman_task_result(second_task);

  test_compare(GEARMAN_SUCCESS, gearman_task_return(first_task));
  test_compare(GEARMAN_SUCCESS, gearman_task_return(second_task));

  test_compare(gearman_result_value(first_result), gearman_result_value(second_result));

  gearman_task_free(first_task);
  gearman_task_free(second_task);

  return TEST_SUCCESS;
}

test_return_t unique_compare_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  size_t job_length;

  gearman_string_t unique= { test_literal_param("my little unique") };

  void *job_result= gearman_client_do(client,
                                      worker_function, // function
                                      gearman_c_str(unique),  // unique
                                      gearman_string_param(unique), //workload
                                      &job_length, // result size
                                      &rc);

  test_compare_hint(GEARMAN_SUCCESS, rc, gearman_client_error(client) ? gearman_client_error(client) : gearman_strerror(rc));
  test_compare(gearman_size(unique), job_length);
  test_memcmp(gearman_c_str(unique), job_result, job_length);

  free(job_result);

  return TEST_SUCCESS;
}

test_return_t gearman_client_unique_status_TEST(void *object)
{
  return TEST_SKIPPED;
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;

  gearman_client_add_options(client, GEARMAN_CLIENT_NON_BLOCKING);

  Client client_one(client);
  Client client_two(client);
  Client client_three(client);
  Client client_four(client);

  const char* unique_handle= "local_handle4";

  gearman_return_t ret;
  // First task
  gearman_task_st *first_task= gearman_client_add_task(&client_one,
                                                       NULL, // preallocated task
                                                       NULL, // context 
                                                       __func__, // function
                                                       unique_handle, // unique
                                                       test_literal_param("first_task"), // workload
                                                       &ret);
  test_true(first_task);

  gearman_task_st *second_task= gearman_client_add_task(&client_two,
                                                        NULL, // preallocated task
                                                        NULL, // context 
                                                        __func__, // function
                                                        unique_handle, // unique
                                                        test_literal_param("second_task"), // workload
                                                        &ret);
  test_true(second_task);

  gearman_task_st *third_task= gearman_client_add_task(&client_three,
                                                       NULL, // preallocated task
                                                       NULL, // context 
                                                       __func__, // function
                                                       unique_handle, // unique
                                                       test_literal_param("third_task"), // workload
                                                       &ret);
  test_true(third_task);

  test_compare(GEARMAN_SUCCESS, gearman_client_set_server_option(client, test_literal_param("marker")));

  size_t limit= 4;
  do {
    ret= gearman_client_run_tasks(&client_one);
  } while (gearman_continue(ret) and limit--);

  limit= 4;
  do {
    ret= gearman_client_run_tasks(&client_two);
  } while (gearman_continue(ret) and limit--);

  limit= 4;
  do {
    ret= gearman_client_run_tasks(&client_three);
  } while (gearman_continue(ret) and limit--);

  gearman_status_t status= gearman_client_unique_status(&client_four,
                                                        unique_handle, strlen(unique_handle));

  test_compare(GEARMAN_SUCCESS, status.result_rc);

  gearman_task_free(first_task);
  gearman_task_free(second_task);
  gearman_task_free(third_task);

  return TEST_SUCCESS;
}
