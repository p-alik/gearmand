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

#include <libtest/test.h>
#include <cassert>
#include <cstring>
#include <libgearman/gearman.h>
#include <string>
#include <tests/execute.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

test_return_t gearman_client_execute_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  assert(worker_function);

  gearman_workload_t workload= gearman_workload_make();

  gearman_task_st *task;
  gearman_argument_t value= gearman_argument_make(gearman_literal_param("test load"));

  test_true_got(task= gearman_client_execute(client, gearman_c_str_param(worker_function), NULL, 0, &workload, &value), gearman_client_error(client));
  test_compare(gearman_literal_param_size("test load"), gearman_result_size(gearman_task_result(task)));

  gearman_task_free(task);

  return TEST_SUCCESS;
}

test_return_t gearman_client_execute_timeout_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  assert(worker_function);
  test_truth(client);
  gearman_workload_t workload= gearman_workload_make();

  gearman_client_set_timeout(client, 4);

  // We should fail since the the timeout is small and the function should
  // not exist.
  gearman_task_st *task;
  gearman_argument_t value= gearman_argument_make(gearman_literal_param("test load"));
  test_true_got(task= gearman_client_execute(client, gearman_c_str_param(worker_function), NULL, 0, &workload, &value), gearman_client_error(client));
  gearman_task_free(task);

  return TEST_SUCCESS;
}

test_return_t gearman_client_execute_epoch_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  assert(worker_function);

  gearman_workload_t workload= gearman_workload_make();
  gearman_workload_set_epoch(&workload, time(NULL) +5);

  gearman_task_st *task;
  gearman_argument_t value= gearman_argument_make(gearman_literal_param("test load"));
  test_true_got(task= gearman_client_execute(client, gearman_c_str_param(worker_function), NULL, 0, &workload, &value), gearman_client_error(client));
  test_truth(task);
  test_truth(gearman_task_job_handle(task));
  gearman_task_free(task);

  return TEST_SUCCESS;
}

test_return_t gearman_client_execute_bg_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  assert(worker_function);

  gearman_workload_t workload= gearman_workload_make();
  gearman_workload_set_background(&workload, true);

  gearman_task_st *task;
  gearman_argument_t value= gearman_argument_make(gearman_literal_param("test load"));
  test_true_got(task= gearman_client_execute(client, gearman_c_str_param(worker_function), gearman_literal_param("my id"), &workload, &value), gearman_client_error(client));

  // Lets make sure we have a task
  test_truth(task);
  test_truth(gearman_task_job_handle(task));

  test_true_got(gearman_success(gearman_client_run_tasks(client)), gearman_client_error(client));

  gearman_task_free(task);

  return TEST_SUCCESS;
}

test_return_t gearman_client_execute_multile_bg_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  assert(worker_function);

  for (uint32_t x= 0; x < 4; /* No reason for number */ x++)
  {
    gearman_workload_t workload= gearman_workload_make();
    gearman_workload_set_background(&workload, true);

    gearman_task_st *task;
    gearman_argument_t value= gearman_argument_make(gearman_literal_param("test load"));
    test_true_got(task= gearman_client_execute(client, gearman_c_str_param(worker_function), NULL, 0, &workload, &value), gearman_client_error(client));
    
    // Lets make sure we have a task
    test_truth(task);
    test_truth(gearman_task_job_handle(task));
  }

  gearman_return_t rc;
  test_true_got(gearman_success(rc= gearman_client_run_tasks(client)), gearman_strerror(rc));

  gearman_client_task_free_all(client);

  return TEST_SUCCESS;
}
