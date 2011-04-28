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
#include <tests/execute.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

test_return_t gearman_client_execute_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  gearman_function_st *function= gearman_function_create(gearman_string_param(worker_function));

  gearman_workload_t workload= gearman_workload_make(gearman_literal_param("test load"));

  test_true_got(gearman_client_execute(client, function, &workload), gearman_client_error(client));

  gearman_task_st *task= gearman_workload_task(&workload);
  test_truth(task);
  test_compare(gearman_workload_size(&workload), gearman_task_result_size(task));

  gearman_task_free(task);
  gearman_function_free(function);

  return TEST_SUCCESS;
}

test_return_t gearman_client_execute_timeout_test(void *object)
{
  gearman_client_st *original= (gearman_client_st *)object;
  gearman_client_st *client= gearman_client_clone(NULL, original);
  test_truth(client);
  gearman_function_st *function= gearman_function_create(gearman_literal_param("no_worker_should_be_found"));
  gearman_workload_t workload= gearman_workload_make(gearman_literal_param("test load"));

  int timeout= gearman_client_timeout(client);
  gearman_client_set_timeout(client, 4);

  // We should fail since the the timeout is small and the function should
  // not exist.
  test_true_got(not gearman_client_execute(client, function, &workload), gearman_client_error(client));
  gearman_client_set_timeout(client, timeout);

  gearman_function_free(function);
  gearman_client_free(client);

  return TEST_SUCCESS;
}

test_return_t gearman_client_execute_epoch_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  gearman_function_st *function= gearman_function_create(gearman_string_param(worker_function));

  gearman_workload_t workload= gearman_workload_make(gearman_literal_param("test load"));
  gearman_workload_set_epoch(&workload, time(NULL) +5);

  test_true_got(gearman_client_execute(client, function, &workload), gearman_client_error(client));
  gearman_task_st *task= gearman_workload_task(&workload);
  test_truth(task);
  test_truth(gearman_task_job_handle(task));
  gearman_task_free(task);

  gearman_function_free(function);

  return TEST_SUCCESS;
}

test_return_t gearman_client_execute_bg_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  gearman_function_st *function= gearman_function_create(gearman_string_param(worker_function));

  gearman_workload_t workload= gearman_workload_make_unique(gearman_literal_param("test load"), gearman_literal_param("my id"));
  gearman_workload_set_background(&workload, true);

  test_true_got(gearman_client_execute(client, function, &workload), gearman_client_error(client));

  // Lets make sure we have a task
  gearman_task_st *task= gearman_workload_task(&workload);
  test_truth(task);
  test_truth(gearman_task_job_handle(task));

  gearman_return_t rc;
  do {
    rc= gearman_client_run_tasks(client);
  } while (rc == GEARMAN_PAUSE);
  test_false(gearman_task_is_running(task));

  gearman_task_free(task);

  gearman_function_free(function);

  return TEST_SUCCESS;
}

test_return_t gearman_client_execute_multile_bg_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  gearman_function_st *function= gearman_function_create(gearman_string_param(worker_function));

  for (uint32_t x= 0; x < 4; /* No reason for number */ x++)
  {
    gearman_workload_t workload= gearman_workload_make(gearman_literal_param("test load"));
    gearman_workload_set_background(&workload, true);

    test_true_got(gearman_client_execute(client, function, &workload), gearman_client_error(client));
    
    // Lets make sure we have a task
    gearman_task_st *task= gearman_workload_task(&workload);
    test_truth(task);
    test_truth(gearman_task_job_handle(task));
  }

  gearman_return_t rc;
  do {
    rc= gearman_client_run_tasks(client);
  } while (rc == GEARMAN_PAUSE);

  gearman_client_task_free_all(client);

  gearman_function_free(function);

  return TEST_SUCCESS;
}

test_return_t gearman_client_execute_in_bulk(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  gearman_function_st *function= gearman_function_create(gearman_string_param(worker_function));

  gearman_workload_t workload[3]= { 
    gearman_workload_make(gearman_literal_param("test load")),
    gearman_workload_make(gearman_literal_param("test load2")),
    gearman_workload_make(gearman_literal_param("test load3"))
  };

  gearman_workload_t *ptr= workload;
  for (uint32_t x= 0; x < gearman_workload_array_size(workload); x++, ptr++)
  {
    gearman_workload_set_background(ptr, true);
  }

  test_true_got(gearman_client_execute(client, function, workload), gearman_client_error(client));

  // Lets make sure we have a task
  gearman_task_st *task= gearman_workload_task(&workload[0]);
  test_truth(task);
  test_truth(gearman_task_job_handle(task));

  gearman_return_t rc;
  do {
    rc= gearman_client_run_tasks(client);
  } while (rc == GEARMAN_PAUSE);

  gearman_client_task_free_all(client);

  gearman_function_free(function);

  return TEST_SUCCESS;
}

test_return_t gearman_client_execute_in_bulk_fail(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;
  const char *worker_function= (const char *)gearman_client_context(client);
  gearman_function_st *function= gearman_function_create(gearman_string_param(worker_function));

  gearman_workload_t workload[]= { 
    gearman_workload_make(gearman_literal_param("test load")),
    gearman_workload_make(gearman_literal_param("test load2")),
    gearman_workload_make(gearman_literal_param("fail")),
    gearman_workload_make(gearman_literal_param("test load3"))
  };

  test_true_got(gearman_client_execute(client, function, workload), gearman_client_error(client));

  gearman_workload_t *ptr= workload;
  for (uint32_t x= 0; x < gearman_workload_array_size(workload); x++, ptr++)
  {
    if (gearman_workload_compare(ptr, gearman_literal_param("fail")))
    {
      gearman_task_st *task= gearman_workload_task(ptr);
      test_true_got(task or not memcmp(ptr->c_str, gearman_literal_param("fail")), ptr->c_str);
      test_true_got(gearman_failed(gearman_task_error(task)), gearman_strerror(gearman_task_error(task)));
    }
  }

  gearman_client_task_free_all(client);

  gearman_function_free(function);

  return TEST_SUCCESS;
}
