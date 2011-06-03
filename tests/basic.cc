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
#include <cstring>

#include <libgearman/gearman.h>

#include <tests/basic.h>
#include <tests/context.h>
#include <libtest/worker.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
pthread_mutex_t counter_lock= PTHREAD_MUTEX_INITIALIZER;

/* Counter test for worker */
static void *counter_function(gearman_job_st *,
                              void *context, size_t *result_size,
                              gearman_return_t *)
{
  uint32_t *counter= (uint32_t *)context;

  *result_size= 0;

  pthread_mutex_lock(&counter_lock);
  *counter= *counter + 1;
  pthread_mutex_unlock(&counter_lock);

  return NULL;
}

test_return_t client_echo_fail_test(void *object)
{
  Context *test= (Context *)object;
  test_truth(test);

  gearman_client_st client, *client_ptr;

  client_ptr= gearman_client_create(&client);
  test_truth(client_ptr);

  test_true_got(gearman_success(gearman_client_add_server(&client, NULL, 20)), gearman_client_error(client_ptr));

  gearman_return_t rc= gearman_client_echo(&client, gearman_literal_param("This should never work"));
  test_true_got(gearman_failed(rc), gearman_strerror(rc));

  gearman_client_free(&client);

  return TEST_SUCCESS;
}

test_return_t client_echo_test(void *object)
{
  Context *test= (Context *)object;
  test_truth(test);

  gearman_client_st client, *client_ptr;

  client_ptr= gearman_client_create(&client);
  test_truth(client_ptr);

  test_true_got(gearman_success(gearman_client_add_server(&client, NULL, test->port())), gearman_client_error(client_ptr));

  gearman_return_t rc= gearman_client_echo(&client, gearman_literal_param("This is my echo test"));
  test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));

  gearman_client_free(&client);

  return TEST_SUCCESS;
}

test_return_t worker_echo_test(void *object)
{
  Context *test= (Context *)object;
  test_truth(test);

  gearman_worker_st *worker= test->worker;
  test_truth(worker);

  gearman_return_t rc= gearman_worker_echo(worker, gearman_literal_param("This is my echo test"));
  test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));

  return TEST_SUCCESS;
}

test_return_t queue_clean(void *object)
{
  Context *test= (Context *)object;
  test_truth(test);
  gearman_worker_st *worker= test->worker;
  test_truth(worker);

  gearman_worker_set_timeout(worker, 200);

  uint32_t counter= 0;
  test_true_got(gearman_success(gearman_worker_add_function(worker, test->worker_function_name(), 5, counter_function, &counter)), gearman_worker_error(worker));

  // Clean out any jobs that might still be in the queue from failed tests.
  while (1)
  {
    gearman_return_t rc= gearman_worker_work(worker);
    if (rc != GEARMAN_SUCCESS)
      break;
  }

  return TEST_SUCCESS;
}

test_return_t queue_add(void *object)
{
  Context *test= (Context *)object;
  gearman_client_st client, *client_ptr;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE]= {};
  test_truth(test);

  test->run_worker= false;

  client_ptr= gearman_client_create(&client);
  test_truth(client_ptr);

  test_true_got(gearman_success(gearman_client_add_server(&client, NULL, test->port())), gearman_client_error(client_ptr));

  gearman_return_t rc= gearman_client_echo(&client, gearman_literal_param("background_payload"));
  test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));

  rc= gearman_client_do_background(&client, test->worker_function_name(), NULL, 
                                   gearman_literal_param("background_payload"),
                                   job_handle);
  test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));
  test_truth(job_handle[0]);

  gearman_client_free(&client);

  test->run_worker= true;

  return TEST_SUCCESS;
}

test_return_t queue_worker(void *object)
{
  Context *test= (Context *)object;
  test_truth(test);

  // Setup job
  test_truth(queue_add(object) == TEST_SUCCESS);

  gearman_worker_st *worker= test->worker;
  test_truth(worker);

  test_true_got(test->run_worker, "run_worker was not set");

  uint32_t counter= 0;
  gearman_return_t rc= gearman_worker_add_function(worker, test->worker_function_name(), 5, counter_function, &counter);
  test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));

  gearman_worker_set_timeout(worker, 5);

  rc= gearman_worker_work(worker);
  test_true_got(rc == GEARMAN_SUCCESS, (rc == GEARMAN_TIMEOUT) ? "Worker was not able to connection to the server, is it running?": gearman_strerror(rc));

  if (rc == GEARMAN_SUCCESS)
  {
    test_truth (counter != 0);
  }

  return TEST_SUCCESS;
}

#define NUMBER_OF_WORKERS 10
#define NUMBER_OF_JOBS 40
#define JOB_SIZE 100
test_return_t lp_734663(void *object)
{
  Context *test= (Context *)object;
  test_truth(test);

  const char *worker_function_name= "drizzle_queue_test";

  uint8_t value[JOB_SIZE]= { 'x' };
  memset(&value, 'x', sizeof(value));

  gearman_client_st client, *client_ptr;
  client_ptr= gearman_client_create(&client);
  test_truth(client_ptr);

  test_true_got(gearman_success(gearman_client_add_server(&client, NULL, test->port())), gearman_client_error(client_ptr));

  uint32_t echo_loop= 3;
  do {
    if (echo_loop != 3)
      sleep(1); // Yes, rigging sleep() in order to make sue the server is there

    gearman_return_t rc= gearman_client_echo(&client, value, sizeof(JOB_SIZE));
    test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));
  } while (--echo_loop);

  for (uint32_t x= 0; x < NUMBER_OF_JOBS; x++)
  {
    char job_handle[GEARMAN_JOB_HANDLE_SIZE]= {};
    gearman_return_t rc= gearman_client_do_background(&client, worker_function_name, NULL, value, sizeof(value), job_handle);
    test_truth(rc == GEARMAN_SUCCESS);
    test_truth(job_handle[0]);
  }

  gearman_client_free(&client);

  struct worker_handle_st *worker_handle[NUMBER_OF_WORKERS];

  uint32_t counter= 0;
  for (uint32_t x= 0; x < NUMBER_OF_WORKERS; x++)
  {
    worker_handle[x]= test_worker_start(test->port(), worker_function_name, counter_function, &counter, gearman_worker_options_t());
  }

  time_t end_time= time(NULL) + 5;
  time_t current_time= 0;
  while (counter < NUMBER_OF_JOBS || current_time < end_time)
  {
    sleep(1);
    current_time= time(NULL);
  }

  for (uint32_t x= 0; x < NUMBER_OF_WORKERS; x++)
  {
    test_worker_stop(worker_handle[x]);
  }

  return TEST_SUCCESS;
}
