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

#pragma GCC diagnostic ignored "-Wold-style-cast"

/* Counter test for worker */
static void *counter_function(gearman_job_st *job __attribute__((unused)),
                              void *context, size_t *result_size,
                              gearman_return_t *ret_ptr __attribute__((unused)))
{
  uint32_t *counter= (uint32_t *)context;

  *result_size= 0;

  *counter= *counter + 1;

  return NULL;
}

test_return_t echo_test(void *object)
{
  gearman_client_st client, *client_ptr;
  gearman_return_t rc;
  const char *value= "This is my echo test";
  size_t value_length= sizeof("This is my echo test") -1;

  (void)object;

  client_ptr= gearman_client_create(&client);
  test_truth(client_ptr);

  rc= gearman_client_echo(&client, (uint8_t *)value, value_length);

  test_truth(rc == GEARMAN_SUCCESS);

  gearman_client_free(&client);

  return TEST_SUCCESS;
}

test_return_t queue_clean(void *object)
{
  Context *test= (Context *)object;
  gearman_worker_st *worker= test->worker;
  uint32_t counter= 0;
  gearman_return_t rc;

  gearman_worker_set_timeout(worker, 200);
  rc= gearman_worker_add_function(worker, "queue_test", 5, counter_function, &counter);
  test_truth(rc == GEARMAN_SUCCESS);

  // Clean out any jobs that might still be in the queue from failed tests.
  while (1)
  {
    rc= gearman_worker_work(worker);
    if (rc != GEARMAN_SUCCESS)
      break;
  }

  return TEST_SUCCESS;
}

test_return_t queue_add(void *object)
{
  Context *test= (Context *)object;
  gearman_client_st client, *client_ptr;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  uint8_t *value= (uint8_t *)"background_test";
  size_t value_length= strlen("background_test");
  gearman_return_t rc;

  test->run_worker= false;

  client_ptr= gearman_client_create(&client);
  test_truth(client_ptr);

  rc= gearman_client_add_server(&client, NULL, test->port());
  test_truth(rc == GEARMAN_SUCCESS);

  rc= gearman_client_echo(&client, (uint8_t *)value, value_length);
  test_truth(rc == GEARMAN_SUCCESS);

  rc= gearman_client_do_background(&client, "queue_test", NULL, value,
                                   value_length, job_handle);
  test_truth(rc == GEARMAN_SUCCESS);

  gearman_client_free(&client);

  test->run_worker= true;

  return TEST_SUCCESS;
}

#include <iostream>

test_return_t queue_worker(void *object)
{
  Context *test= (Context *)object;
  gearman_worker_st *worker= test->worker;
  uint32_t counter= 0;
  gearman_return_t rc;

  if (! test->run_worker)
    return TEST_FAILURE;

  rc= gearman_worker_add_function(worker, "queue_test", 5, counter_function, &counter);
  test_truth(rc == GEARMAN_SUCCESS);

  gearman_worker_set_timeout(worker, 5);

  std::cerr << "got here " << std::endl;
  rc= gearman_worker_work(worker);
  test_truth(rc == GEARMAN_SUCCESS or rc == GEARMAN_TIMEOUT);
  std::cerr << "gearman_worker_work() " << std::endl;

  if (rc == GEARMAN_SUCCESS)
  {
    test_truth (counter != 0);
  }

  return TEST_SUCCESS;
}
