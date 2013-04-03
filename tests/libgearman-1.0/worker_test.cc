/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011-2013 Data Differential, http://datadifferential.com/
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


#include "gear_config.h"
#include <libtest/test.hpp>

using namespace libtest;

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <libgearman/gearman.h>
#include <libgearman/connection.hpp>
#include "libgearman/packet.hpp"
#include "libgearman/universal.hpp"
#include "libgearman/is.hpp"
#include "libgearman/interface/worker.hpp"

#include "libgearman/client.hpp"
#include "libgearman/worker.hpp"
using namespace org::gearmand;

#include "tests/start_worker.h"
#include "tests/workers/v2/echo_or_react.h"
#include "tests/workers/v2/echo_or_react_chunk.h"

static test_return_t init_test(void *)
{
  gearman_worker_st worker;

  test_truth(gearman_worker_create(&worker));

  gearman_worker_free(&worker);

  return TEST_SUCCESS;
}

static test_return_t allocation_test(void *)
{
  gearman_worker_st *worker;

  test_truth(worker= gearman_worker_create(NULL));

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

static test_return_t sanity_TEST(void *)
{
  // Sanity test on initial enum
  ASSERT_EQ(0, int(GEARMAN_SUCCESS));
  ASSERT_EQ(1, int(GEARMAN_IO_WAIT));

  return TEST_SUCCESS;
}

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static test_return_t gearman_worker_clone_NULL_NULL(void *)
{
  gearman_worker_st *worker= gearman_worker_clone(NULL, NULL);

  test_truth(worker);
  ASSERT_EQ(true, gearman_is_allocated(worker));

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_clone_NULL_SOURCE(void *)
{
  libgearman::Worker source;

  gearman_worker_st *worker= gearman_worker_clone(NULL, &source);
  test_truth(worker);
  ASSERT_EQ(true, gearman_is_allocated(worker));
  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_timeout_default_test(void *)
{
  libgearman::Worker worker;

  ASSERT_EQ(-1, gearman_worker_timeout(&worker));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_free_TEST(void *)
{
  gearman_worker_free(NULL);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_error_TEST(void *)
{
  test_null(gearman_worker_error(NULL));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_error_no_error_TEST(void *)
{
  libgearman::Worker worker;
  test_null(gearman_worker_error(&worker));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_errno_TEST(void *)
{
  ASSERT_EQ(EINVAL, gearman_worker_errno(NULL));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_errno_no_error_TEST(void *)
{
  libgearman::Worker worker;
  ASSERT_EQ(0, gearman_worker_errno(&worker));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_options_TEST(void *)
{
  ASSERT_EQ(gearman_worker_options_t(), gearman_worker_options(NULL));

  return TEST_SUCCESS;
}

static test_return_t option_test(void *)
{
  gearman_worker_options_t default_options;

  gearman_worker_st* gear= gearman_worker_create(NULL);
  test_truth(gear);
  { // Initial Allocated, no changes
    test_truth(gearman_is_allocated(gear));
    test_false(gearman_is_non_blocking(gear->impl()));
    test_truth(gear->impl()->options.packet_init);
    test_false(gear->impl()->options.change);
    test_true(gear->impl()->options.grab_uniq);
    test_false(gear->impl()->options.timeout_return);
  }

  /* Set up for default options */
  default_options= gearman_worker_options(gear);

  /*
    We take the basic options, and push
    them back in. See if we change anything.
  */
  gearman_worker_set_options(gear, default_options);
  { // Initial Allocated, no changes
    test_truth(gearman_is_allocated(gear));
    test_false(gearman_is_non_blocking(gear->impl()));
    test_truth(gear->impl()->options.packet_init);
    test_false(gear->impl()->options.change);
    test_true(gear->impl()->options.grab_uniq);
    test_false(gear->impl()->options.timeout_return);
  }

  /*
    We will trying to modify non-mutable options (which should not be allowed)
  */
  {
    gearman_worker_remove_options(gear, GEARMAN_WORKER_ALLOCATED);
    { // Initial Allocated, no changes
      test_truth(gearman_is_allocated(gear));
      test_false(gearman_is_non_blocking(gear->impl()));
      test_truth(gear->impl()->options.packet_init);
      test_false(gear->impl()->options.change);
      test_true(gear->impl()->options.grab_uniq);
      test_false(gear->impl()->options.timeout_return);
    }

    gearman_worker_remove_options(gear, GEARMAN_WORKER_PACKET_INIT);
    { // Initial Allocated, no changes
      test_truth(gearman_is_allocated(gear));
      test_false(gearman_is_non_blocking(gear->impl()));
      test_truth(gear->impl()->options.packet_init);
      test_false(gear->impl()->options.change);
      test_true(gear->impl()->options.grab_uniq);
      test_false(gear->impl()->options.timeout_return);
    }
  }

  /*
    We will test modifying GEARMAN_WORKER_NON_BLOCKING in several manners.
  */
  {
    gearman_worker_remove_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gearman_is_allocated(gear));
      test_false(gearman_is_non_blocking(gear->impl()));
      test_truth(gear->impl()->options.packet_init);
      test_false(gear->impl()->options.change);
      test_true(gear->impl()->options.grab_uniq);
      test_false(gear->impl()->options.timeout_return);
    }
    gearman_worker_add_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gearman_is_allocated(gear));
      test_truth(gearman_is_non_blocking(gear->impl()));
      test_truth(gear->impl()->options.packet_init);
      test_false(gear->impl()->options.change);
      test_true(gear->impl()->options.grab_uniq);
      test_false(gear->impl()->options.timeout_return);
    }
    gearman_worker_set_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gearman_is_allocated(gear));
      test_truth(gearman_is_non_blocking(gear->impl()));
      test_truth(gear->impl()->options.packet_init);
      test_false(gear->impl()->options.change);
      test_false(gear->impl()->options.grab_uniq);
      test_false(gear->impl()->options.timeout_return);
    }
    gearman_worker_set_options(gear, GEARMAN_WORKER_GRAB_UNIQ);
    { // Everything is now set to false except GEARMAN_WORKER_GRAB_UNIQ, and non-mutable options
      test_truth(gearman_is_allocated(gear));
      test_false(gearman_is_non_blocking(gear->impl()));
      test_truth(gear->impl()->options.packet_init);
      test_false(gear->impl()->options.change);
      test_truth(gear->impl()->options.grab_uniq);
      test_false(gear->impl()->options.timeout_return);
    }
    /*
      Reset options to default. Then add an option, and then add more options. Make sure
      the options are all additive.
    */
    {
      gearman_worker_set_options(gear, default_options);
      { // See if we return to defaults
        test_truth(gearman_is_allocated(gear));
        test_false(gearman_is_non_blocking(gear->impl()));
        test_truth(gear->impl()->options.packet_init);
        test_false(gear->impl()->options.change);
        test_true(gear->impl()->options.grab_uniq);
        test_false(gear->impl()->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN);
      { // All defaults, except timeout_return
        test_truth(gearman_is_allocated(gear));
        test_false(gearman_is_non_blocking(gear->impl()));
        test_truth(gear->impl()->options.packet_init);
        test_false(gear->impl()->options.change);
        test_true(gear->impl()->options.grab_uniq);
        test_truth(gear->impl()->options.timeout_return);
      }
      gearman_worker_add_options(gear, (gearman_worker_options_t)(GEARMAN_WORKER_NON_BLOCKING|GEARMAN_WORKER_GRAB_UNIQ));
      { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
        test_truth(gearman_is_allocated(gear));
        test_truth(gearman_is_non_blocking(gear->impl()));
        test_truth(gear->impl()->options.packet_init);
        test_false(gear->impl()->options.change);
        test_truth(gear->impl()->options.grab_uniq);
        test_truth(gear->impl()->options.timeout_return);
      }
    }
    /*
      Add an option, and then replace with that option plus a new option.
    */
    {
      gearman_worker_set_options(gear, default_options);
      { // See if we return to defaults
        test_truth(gearman_is_allocated(gear));
        test_false(gearman_is_non_blocking(gear->impl()));
        test_truth(gear->impl()->options.packet_init);
        test_false(gear->impl()->options.change);
        test_true(gear->impl()->options.grab_uniq);
        test_false(gear->impl()->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN);
      { // All defaults, except timeout_return
        test_truth(gearman_is_allocated(gear));
        test_false(gearman_is_non_blocking(gear->impl()));
        test_truth(gear->impl()->options.packet_init);
        test_false(gear->impl()->options.change);
        test_true(gear->impl()->options.grab_uniq);
        test_truth(gear->impl()->options.timeout_return);
      }
      gearman_worker_add_options(gear, (gearman_worker_options_t)(GEARMAN_WORKER_TIMEOUT_RETURN|GEARMAN_WORKER_GRAB_UNIQ));
      { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
        test_truth(gearman_is_allocated(gear));
        test_false(gearman_is_non_blocking(gear->impl()));
        test_truth(gear->impl()->options.packet_init);
        test_false(gear->impl()->options.change);
        test_truth(gear->impl()->options.grab_uniq);
        test_truth(gear->impl()->options.timeout_return);
      }
    }
  }

  gearman_worker_free(gear);

  return TEST_SUCCESS;
}

static test_return_t echo_test(void*)
{
  libgearman::Worker worker;

  ASSERT_EQ(gearman_worker_echo(&worker, test_literal_param("This is my echo test")), GEARMAN_NO_SERVERS);
  ASSERT_EQ(GEARMAN_SUCCESS, gearman_worker_add_server(&worker, "localhost", libtest::default_port()));
  ASSERT_EQ(GEARMAN_SUCCESS, gearman_worker_echo(&worker, test_literal_param("This is my echo test")));

  return TEST_SUCCESS;
}

static test_return_t echo_multi_test(void *)
{
  libgearman::Worker worker;
  ASSERT_EQ(GEARMAN_SUCCESS, gearman_worker_add_server(&worker, "localhost", libtest::default_port()));

  const char *value[]= {
    "This is my echo test",
    "This land is my land",
    "This land is your land",
    "We the people",
    "in order to form a more perfect union",
    "establish justice",
    NULL
  };
  const char **ptr= value;

  while (*ptr)
  {
    ASSERT_EQ(gearman_worker_echo(&worker, test_string_make_from_cstr(*ptr)), GEARMAN_SUCCESS);
    ptr++;
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_server_GEARMAN_INVALID_ARGUMENT_TEST(void *)
{
  if (libtest::check_dns())
  {
    ASSERT_EQ(GEARMAN_INVALID_ARGUMENT,
                 gearman_worker_add_server(NULL, "nonexist.gearman.info", libtest::default_port()));
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_server_GEARMAN_GETADDRINFO_TEST(void *)
{
  if (libtest::check_dns())
  {
    gearman_worker_st *worker= gearman_worker_create(NULL);
    test_true(worker);
    ASSERT_EQ(gearman_worker_add_server(worker, "nonexist.gearman.info", libtest::default_port()), GEARMAN_GETADDRINFO);
    gearman_worker_free(worker);
  }

  return TEST_SUCCESS;
}

static test_return_t echo_max_test(void *)
{
  libgearman::Worker worker(libtest::default_port());;

  ASSERT_EQ(GEARMAN_ARGUMENT_TOO_LARGE,
               gearman_worker_echo(&worker, "This is my echo test", GEARMAN_MAX_ECHO_SIZE +1));

  return TEST_SUCCESS;
}

// The idea is to return GEARMAN_ERROR until we hit limit, then return
// GEARMAN_SUCCESS
static gearman_return_t GEARMAN_ERROR_worker(gearman_job_st* job, void *context)
{
  assert(gearman_job_workload_size(job) == 0);
  assert(gearman_job_workload(job) == NULL);
  size_t *ret= (size_t*)context;

  if (*ret > 0)
  {
    *ret= (*ret) -1;
    return GEARMAN_ERROR;
  }

  if (gearman_failed(gearman_job_send_data(job, test_literal_param("OK"))))
  {
    // We should return ERROR here, but that would then possibly loop
    return GEARMAN_FAIL;
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t error_return_worker(gearman_job_st* job, void *)
{
  assert(sizeof(gearman_return_t) == gearman_job_workload_size(job));
  const gearman_return_t *ret= (const gearman_return_t*)gearman_job_workload(job);

  if (gearman_failed(gearman_job_send_data(job, gearman_strerror(*ret), strlen(gearman_strerror(*ret)))))
  {
    return GEARMAN_ERROR;
  }

  return *ret;
}

static test_return_t error_return_TEST(void *)
{
  libgearman::Client client(libtest::default_port());
  ASSERT_EQ(GEARMAN_SUCCESS, gearman_client_echo(&client, test_literal_param(__func__)));

  libgearman::Worker worker(libtest::default_port());
  ASSERT_EQ(gearman_worker_register(&worker, __func__, 0), GEARMAN_SUCCESS);

  gearman_task_attr_t task_attr= gearman_task_attr_init_background(GEARMAN_JOB_PRIORITY_NORMAL);

  std::vector<std::string> job_handles;
  job_handles.resize(int(GEARMAN_MAX_RETURN));
  for (gearman_return_t x= GEARMAN_IO_WAIT; int(x) < int(GEARMAN_MAX_RETURN); x= gearman_return_t((int(x) +1)))
  {
    if (x == GEARMAN_SHUTDOWN)
    {
      continue;
    }

    if (x == GEARMAN_WORK_ERROR)
    {
      continue;
    }

    gearman_argument_t arg= gearman_argument_make(NULL, 0, (const char*)&x, sizeof(gearman_return_t));
    gearman_task_st *task= gearman_execute(&client,
                                           test_literal_param(__func__),
                                           NULL, 0, // unique
                                           &task_attr, // gearman_task_attr_t
                                           &arg, // gearman_argument_t
                                           NULL); // context
    test_truth(task);

    bool is_known;
    ASSERT_EQ(gearman_client_job_status(&client, gearman_task_job_handle(task), &is_known, NULL, NULL, NULL), GEARMAN_SUCCESS);
    test_true(is_known);
    job_handles[int(x)].append(gearman_task_job_handle(task));
  }

  gearman_function_t error_return_TEST_FN= gearman_function_create(error_return_worker);
  std::auto_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(),
                                                           NULL,
                                                           __func__,
                                                           error_return_TEST_FN,
                                                           NULL,
                                                           gearman_worker_options_t(),
                                                           0)); // timeout

  for (gearman_return_t x= GEARMAN_IO_WAIT; int(x) < int(GEARMAN_MAX_RETURN); x= gearman_return_t((int(x) +1)))
  {
    if (x == GEARMAN_SHUTDOWN)
    {
      continue;
    }

    if (x == GEARMAN_WORK_ERROR)
    {
      continue;
    }

    bool is_known;
    gearman_return_t rc;
    do {
      rc= gearman_client_job_status(&client, job_handles[int(x)].c_str(), &is_known, NULL, NULL, NULL);
    }  while (gearman_continue(rc) or is_known);
    test_false(handle->is_shutdown());
  }

  return TEST_SUCCESS;
}

static test_return_t GEARMAN_ERROR_return_TEST(void *)
{
  libgearman::Client client(libtest::default_port());
  ASSERT_EQ(GEARMAN_SUCCESS, gearman_client_echo(&client, test_literal_param(__func__)));

  size_t count= 0;
  gearman_function_t GEARMAN_ERROR_FN= gearman_function_create(GEARMAN_ERROR_worker);
  std::auto_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(),
                                                           NULL,
                                                           __func__,
                                                           GEARMAN_ERROR_FN,
                                                           &count,
                                                           gearman_worker_options_t(),
                                                           0)); // timeout

  for (size_t x= 0; x < 24; x++)
  {
    count= x;
    gearman_task_st *task= gearman_execute(&client,
                                           test_literal_param(__func__),
                                           NULL, 0, // unique
                                           NULL, // gearman_task_attr_t
                                           NULL, // gearman_argument_t
                                           NULL); // context
    test_truth(task);

    gearman_return_t rc;
    bool is_known;
    do {
      rc= gearman_client_job_status(&client, gearman_task_job_handle(task), &is_known, NULL, NULL, NULL);
    }  while (gearman_continue(rc) or is_known);

    ASSERT_EQ(gearman_task_return(task), GEARMAN_SUCCESS);
    test_zero(count); // Since we hit zero we know that we ran enough times.

    gearman_result_st *result= gearman_task_result(task);
    test_true(result);
    test_memcmp("OK", gearman_result_value(result), strlen("ok"));
  }

  return TEST_SUCCESS;
}

static test_return_t GEARMAN_FAIL_return_TEST(void *)
{
  libgearman::Client client(libtest::default_port());

  ASSERT_EQ(GEARMAN_SUCCESS, gearman_client_echo(&client, test_literal_param(__func__)));

  gearman_function_t error_return_TEST_FN= gearman_function_create(error_return_worker);
  std::auto_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(),
                                                           NULL,
                                                           __func__,
                                                           error_return_TEST_FN,
                                                           NULL,
                                                           gearman_worker_options_t(),
                                                           0)); // timeout

  int count= 3;
  while(--count)
  {
    gearman_return_t x= GEARMAN_FAIL;
    gearman_argument_t arg= gearman_argument_make(NULL, 0, (const char*)&x, sizeof(gearman_return_t));
    gearman_task_st *task= gearman_execute(&client,
                                           test_literal_param(__func__),
                                           NULL, 0, // unique
                                           NULL, // gearman_task_attr_t
                                           &arg, // gearman_argument_t
                                           NULL); // context
    test_truth(task);

    gearman_return_t rc;
    bool is_known;
    do {
      rc= gearman_client_job_status(&client, gearman_task_job_handle(task), &is_known, NULL, NULL, NULL);
    }  while (gearman_continue(rc) or is_known);

    {
      ASSERT_EQ(GEARMAN_FAIL, gearman_task_return(task));
    }
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_client_job_status_is_known_TEST(void *)
{
  libgearman::Client client(libtest::default_port());
  libgearman::Worker worker(libtest::default_port());

  ASSERT_EQ(gearman_worker_register(&worker, __func__, 0), GEARMAN_SUCCESS);

  gearman_job_handle_t job_handle;
  ASSERT_EQ(gearman_client_do_background(&client, __func__, NULL, NULL, 0, job_handle), GEARMAN_SUCCESS);

  bool is_known;
  ASSERT_EQ(gearman_client_job_status(&client, job_handle, &is_known, NULL, NULL, NULL), GEARMAN_SUCCESS);

  test_true(is_known);

  gearman_function_t echo_or_react_worker_v2_FN= gearman_function_create(echo_or_react_worker_v2);
  std::auto_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(),
                                                           NULL,
                                                           __func__,
                                                           echo_or_react_worker_v2_FN,
                                                           NULL,
                                                           gearman_worker_options_t(),
                                                           0)); // timeout

  return TEST_SUCCESS;
}

static test_return_t abandoned_worker_test(void *)
{
  gearman_job_handle_t job_handle;
  const void *args[2];
  size_t args_size[2];

  {
    libgearman::Client client(libtest::default_port());
    ASSERT_EQ(gearman_client_do_background(&client, "abandoned_worker", NULL, NULL, 0, job_handle),
                 GEARMAN_SUCCESS);
  }

  /* Now take job with one worker. */
  gearman_universal_st universal;

  gearman_connection_st *connection1;
  test_truth(connection1= gearman_connection_create(universal, NULL));

  connection1->set_host(NULL, libtest::default_port());

  gearman_packet_st packet;
  args[0]= "abandoned_worker";
  args_size[0]= strlen("abandoned_worker");
  ASSERT_EQ(GEARMAN_SUCCESS,
               gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                          GEARMAN_COMMAND_CAN_DO,
                                          args, args_size, 1));

  ASSERT_EQ(connection1->send_packet(packet, true),
               GEARMAN_SUCCESS);

  gearman_packet_free(&packet);

  gearman_return_t ret;
  ASSERT_EQ(GEARMAN_SUCCESS,
               gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_GRAB_JOB, NULL, NULL, 0));

  ASSERT_EQ(GEARMAN_SUCCESS, connection1->send_packet(packet, true));

  gearman_packet_free(&packet);

  connection1->receiving(packet, ret, false);
  test_truth(not (ret != GEARMAN_SUCCESS or packet.command != GEARMAN_COMMAND_JOB_ASSIGN));

  test_strcmp(job_handle, packet.arg[0]); // unexepcted job

  gearman_packet_free(&packet);

  gearman_connection_st *connection2;
  test_truth(connection2= gearman_connection_create(universal, NULL));

  connection2->set_host(NULL, libtest::default_port());

  args[0]= "abandoned_worker";
  args_size[0]= strlen("abandoned_worker");
  ASSERT_EQ(GEARMAN_SUCCESS, gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                           GEARMAN_COMMAND_CAN_DO,
                                                           args, args_size, 1));

  ASSERT_EQ(GEARMAN_SUCCESS, connection2->send_packet(packet, true));

  gearman_packet_free(&packet);

  args[0]= job_handle;
  args_size[0]= strlen(job_handle) + 1;
  args[1]= "test";
  args_size[1]= 4;
  ASSERT_EQ(GEARMAN_SUCCESS, gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                           GEARMAN_COMMAND_WORK_COMPLETE,
                                                           args, args_size, 2));

  ASSERT_EQ(GEARMAN_SUCCESS, connection2->send_packet(packet, true));

  gearman_packet_free(&packet);

  gearman_universal_set_timeout(universal, 1000);
  connection2->receiving(packet, ret, false);
  test_truth(not (ret != GEARMAN_SUCCESS or packet.command != GEARMAN_COMMAND_ERROR));

  delete connection1;
  delete connection2;
  gearman_packet_free(&packet);
  gearman_universal_free(universal);

  return TEST_SUCCESS;
}

static void *no_unique_worker(gearman_job_st *job,
                              void *, size_t *size,
                              gearman_return_t *ret_ptr)
{
  if (gearman_job_unique(job) and strlen(gearman_job_unique(job)))
  {
    *ret_ptr= GEARMAN_WORK_FAIL;
  }
  else
  {
    *ret_ptr= GEARMAN_SUCCESS;
  }
  *size= 0;

  return NULL;
}

static void *check_unique_worker(gearman_job_st *job,
                                 void *context, size_t *size,
                                 gearman_return_t *ret_ptr)
{
  if (gearman_job_unique(job))
  {
    size_t length= strlen(gearman_job_unique(job));
    if (length ==  gearman_job_workload_size(job))
    {
      if (not memcmp(gearman_job_unique(job), gearman_job_workload(job),length))
      {
        bool *success= (bool *)context;
        if (success)
          *success= true;

        *ret_ptr= GEARMAN_SUCCESS;
        *size= length;
        return strdup((char*)gearman_job_unique(job));
      }
    }
  }

  *size= 0;
  *ret_ptr= GEARMAN_WORK_FAIL;

  return NULL;
}

static void *fail_worker(gearman_job_st *,
                         void *, size_t *size,
                         gearman_return_t *ret_ptr)
{
  *ret_ptr= GEARMAN_WORK_FAIL;
  *size= 0;

  return NULL;
}

static test_return_t gearman_worker_add_function_test(void *)
{
  libgearman::Worker worker;

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  ASSERT_EQ(GEARMAN_SUCCESS,
               gearman_worker_add_function(&worker, function_name,0, fail_worker, NULL));

  ASSERT_EQ(true, gearman_worker_function_exist(&worker, test_string_make_from_array(function_name)));

  ASSERT_EQ(GEARMAN_SUCCESS,
               gearman_worker_unregister(&worker, function_name));

  ASSERT_EQ(false, gearman_worker_function_exist(&worker, function_name, strlen(function_name)));

  /* Make sure we have removed it */
  ASSERT_EQ(GEARMAN_NO_REGISTERED_FUNCTION, 
               gearman_worker_unregister(&worker, function_name));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_function_multi_test(void *)
{
  libgearman::Worker worker;

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];
    snprintf(buffer, 1024, "%u%s", x, __func__);

    ASSERT_EQ(GEARMAN_SUCCESS,
                 gearman_worker_add_function(&worker, buffer, 0, fail_worker, NULL));
  }

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, 1024, "%u%s", x, __func__);
    ASSERT_EQ(GEARMAN_SUCCESS,
                 gearman_worker_unregister(&worker, buffer));
  }

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, 1024, "%u%s", x, __func__);
    ASSERT_EQ(GEARMAN_NO_REGISTERED_FUNCTION,
                 gearman_worker_unregister(&worker, buffer));
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_unregister_all_test(void *)
{
  libgearman::Worker worker;
  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%u%s", x, __func__);
    gearman_return_t rc= gearman_worker_add_function(&worker,
						     buffer,
						     0, fail_worker, NULL);

    ASSERT_EQ(rc, GEARMAN_SUCCESS);
  }

  ASSERT_EQ(GEARMAN_SUCCESS,
               gearman_worker_unregister_all(&worker));

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, sizeof(buffer), "%u%s", x, __func__);
    gearman_return_t rc= gearman_worker_unregister(&worker, buffer);
    ASSERT_EQ(rc, GEARMAN_NO_REGISTERED_FUNCTION);
  }

  ASSERT_EQ(gearman_worker_unregister_all(&worker),
               GEARMAN_NO_REGISTERED_FUNCTIONS);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_work_with_test(int timeout, gearman_worker_options_t option)
{
  libgearman::Worker worker;

  if (option)
  {
    gearman_worker_add_options(&worker, option);
    if (option == GEARMAN_WORKER_NON_BLOCKING)
    {
      test_true(gearman_worker_options(&worker) & GEARMAN_WORKER_NON_BLOCKING);
    }
  }

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  ASSERT_EQ(gearman_worker_add_function(&worker,
                                           function_name,
                                           0, fail_worker, NULL),
               GEARMAN_SUCCESS);

  gearman_worker_set_timeout(&worker, timeout);

  if (option == GEARMAN_WORKER_NON_BLOCKING)
  {
    ASSERT_EQ(GEARMAN_NO_JOBS,
                 gearman_worker_work(&worker));

    ASSERT_EQ(GEARMAN_NO_JOBS,
                 gearman_worker_work(&worker));
  }
  else
  {
    ASSERT_EQ(GEARMAN_TIMEOUT,
                 gearman_worker_work(&worker));

    ASSERT_EQ(GEARMAN_TIMEOUT,
                 gearman_worker_work(&worker));
  }

  /* Make sure we have removed the worker function */
  ASSERT_EQ(GEARMAN_SUCCESS,
               gearman_worker_unregister(&worker, function_name));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_work_with_option(gearman_worker_options_t option)
{
  int timeout[]= { 500, 1000, 2000, 8000, 0 };

  // First we try with immediate timeout
  ASSERT_EQ(TEST_SUCCESS, gearman_worker_work_with_test(0, option));

  for (size_t x= 0; timeout[x]; ++x)
  {
    ASSERT_EQ(TEST_SUCCESS, gearman_worker_work_with_test(timeout[x], option));
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_work_with_TEST(void*)
{
  return gearman_worker_work_with_option(gearman_worker_options_t());
}

static test_return_t gearman_worker_work_with_GEARMAN_WORKER_NON_BLOCKING_TEST(void*)
{
  return gearman_worker_work_with_option(GEARMAN_WORKER_NON_BLOCKING);
}

static test_return_t gearman_worker_context_test(void *)
{
  libgearman::Worker worker;

  test_false(gearman_worker_context(&worker));

  int value= 5;
  gearman_worker_set_context(&worker, &value);

  int *ptr= (int *)gearman_worker_context(&worker);

  test_truth(ptr == &value);
  test_truth(*ptr == value);
  gearman_worker_set_context(&worker, NULL);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_check_options_GEARMAN_WORKER_GRAB_UNIQ(void *)
{
  libgearman::Worker worker;

  test_true(worker->impl()->options.grab_uniq);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_remove_options_GEARMAN_WORKER_GRAB_UNIQ(void *)
{
  libgearman::Worker worker(libtest::default_port());

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  char unique_name[GEARMAN_MAX_UNIQUE_SIZE];
  snprintf(unique_name, GEARMAN_MAX_UNIQUE_SIZE, "_%s%d", __func__, int(random())); 

  ASSERT_EQ(GEARMAN_SUCCESS,
               gearman_worker_add_function(&worker, function_name, 0, no_unique_worker, NULL));

  {
    libgearman::Client client(libtest::default_port());

    ASSERT_EQ(gearman_client_do_background(&client, function_name, unique_name,
                                              test_string_make_from_array(unique_name), NULL),
                 GEARMAN_SUCCESS);
  }

  gearman_worker_remove_options(&worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_false(worker->impl()->options.grab_uniq);

  gearman_worker_set_timeout(&worker, 800);

  gearman_return_t rc;
  gearman_job_st *job= gearman_worker_grab_job(&worker, NULL, &rc);
  ASSERT_EQ(rc, GEARMAN_SUCCESS);
  test_truth(job);

  size_t size= 0;
  void *result= no_unique_worker(job, NULL, &size, &rc);
  ASSERT_EQ(rc, GEARMAN_SUCCESS);
  test_false(result);
  test_false(size);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ(void *)
{
  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  char unique_name[GEARMAN_MAX_UNIQUE_SIZE];
  snprintf(unique_name, GEARMAN_MAX_UNIQUE_SIZE, "_%s%d", __func__, int(random())); 

  {
    libgearman::Client client(libtest::default_port());

    ASSERT_EQ(gearman_client_do_background(&client, function_name, unique_name,
                                              test_string_make_from_array(unique_name), NULL), 
                 GEARMAN_SUCCESS);
  }

  libgearman::Worker worker(libtest::default_port());

  ASSERT_EQ(GEARMAN_SUCCESS,
               gearman_worker_add_function(&worker, function_name, 0, check_unique_worker, NULL));

  gearman_worker_add_options(&worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_true(worker->impl()->options.grab_uniq);

  gearman_return_t rc;
  gearman_job_st *job= gearman_worker_grab_job(&worker, NULL, &rc);
  ASSERT_EQ(GEARMAN_SUCCESS, rc);
  test_truth(job);

  size_t size= 0;
  void *result= check_unique_worker(job, NULL, &size, &rc);
  ASSERT_EQ(GEARMAN_SUCCESS, rc);
  test_truth(result);
  test_truth(size);
  free(result);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_set_identifier_TEST(void *)
{
  libgearman::Worker worker(libtest::default_port());

  ASSERT_EQ(GEARMAN_SUCCESS,
               gearman_worker_set_identifier(&worker, test_literal_param(__func__)));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ_worker_work(void *)
{
  libgearman::Worker worker(libtest::default_port());

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  char unique_name[GEARMAN_MAX_UNIQUE_SIZE];
  snprintf(unique_name, GEARMAN_MAX_UNIQUE_SIZE, "_%s%d", __func__, int(random())); 

  bool success= false;
  ASSERT_EQ(GEARMAN_SUCCESS,
               gearman_worker_add_function(&worker, function_name, 0, check_unique_worker, &success));

  {
    libgearman::Client client(libtest::default_port());
    ASSERT_EQ(gearman_client_do_background(&client, function_name, unique_name,
                                           test_string_make_from_array(unique_name), NULL),
              GEARMAN_SUCCESS);
  }

  test_true(worker->impl()->options.grab_uniq);
  gearman_worker_add_options(&worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_truth(worker->impl()->options.grab_uniq);

  gearman_worker_set_timeout(&worker, 400);
  ASSERT_EQ(gearman_worker_work(&worker), GEARMAN_SUCCESS);

  test_truth(success);


  return TEST_SUCCESS;
}

static test_return_t _increase_TEST(gearman_function_t &func, gearman_client_options_t options, size_t block_size)
{
  libgearman::Client client(libtest::default_port());

  ASSERT_EQ(GEARMAN_SUCCESS, gearman_client_echo(&client, test_literal_param(__func__)));

  gearman_client_add_options(&client, options);

  std::auto_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(),
                                                           NULL,
                                                           __func__,
                                                           func,
                                                           NULL,
                                                           gearman_worker_options_t(),
                                                           0)); // timeout

  size_t max_block_size= 4;
  if (libtest::is_massive())
  {
    max_block_size= 24;
  }

  for (size_t x= 1; x < max_block_size; ++x)
  {
    if (valgrind_is_caller() and (x * block_size) > 15728640)
    {
      continue;
    }
    libtest::vchar_t workload;
    libtest::vchar::make(workload, x * block_size);

    gearman_argument_t value= gearman_argument_make(0, 0, vchar_param(workload));

    gearman_task_st *task= gearman_execute(&client,
                                           test_literal_param(__func__),
                                           NULL, 0, // unique
                                           NULL, // gearman_task_attr_t
                                           &value, // gearman_argument_t
                                           NULL); // context
    test_truth(task);

    gearman_return_t rc;
    do {
      rc= gearman_client_run_tasks(&client);
      if (options)
      {
        gearman_client_wait(&client);
      }
    }  while (gearman_continue(rc));

    ASSERT_EQ(GEARMAN_SUCCESS,
                 gearman_task_return(task));

    gearman_result_st *result= gearman_task_result(task);
    test_true(result);
    ASSERT_EQ(gearman_result_size(result), workload.size());
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_client_run_tasks_increase_TEST(void*)
{
  gearman_function_t func= gearman_function_create(echo_or_react_worker_v2);
  return _increase_TEST(func, gearman_client_options_t(), 1024 * 1024);
}

static test_return_t gearman_client_run_tasks_increase_GEARMAN_CLIENT_NON_BLOCKING_TEST(void*)
{
  gearman_function_t func= gearman_function_create(echo_or_react_worker_v2);
  return _increase_TEST(func, GEARMAN_CLIENT_NON_BLOCKING, 1024 * 1024);
}

static test_return_t gearman_client_run_tasks_increase_chunk_TEST(void*)
{
  gearman_function_t func= gearman_function_create(echo_or_react_chunk_worker_v2);
  return _increase_TEST(func, gearman_client_options_t(), 1024);
}

static test_return_t gearman_worker_failover_test(void *)
{
  libgearman::Worker worker(libtest::default_port());

  // Now add a port which we do not have a server running on
  ASSERT_EQ(GEARMAN_SUCCESS, gearman_worker_add_server(&worker, NULL, libtest::default_port() +1));

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  ASSERT_EQ(GEARMAN_SUCCESS, 
               gearman_worker_add_function(&worker, function_name, 0, fail_worker, NULL));

  gearman_worker_set_timeout(&worker, 400);

  ASSERT_EQ(GEARMAN_TIMEOUT, gearman_worker_work(&worker));

  /* Make sure we have remove worker function */
  ASSERT_EQ(GEARMAN_SUCCESS,
               gearman_worker_unregister(&worker, function_name));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_set_timeout_FAILOVER_TEST(void *)
{
  test_skip_valgrind(); // lp:961904

  in_port_t known_server_port= libtest::default_port();
  libgearman::Worker worker(known_server_port);

  ASSERT_EQ(GEARMAN_SUCCESS, gearman_worker_add_server(&worker, NULL, known_server_port));

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  ASSERT_EQ(GEARMAN_SUCCESS, 
               gearman_worker_add_function(&worker, function_name, 0, fail_worker, NULL));

  gearman_worker_set_timeout(&worker, 2);

  ASSERT_EQ(GEARMAN_TIMEOUT, gearman_worker_work(&worker));

  /* Make sure we have remove worker function */
  ASSERT_EQ(GEARMAN_SUCCESS,
               gearman_worker_unregister(&worker, function_name));

  return TEST_SUCCESS;
}

/*********************** World functions **************************************/

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (server_startup(servers, "gearmand", libtest::default_port(), NULL) == false)
  {
    error= TEST_SKIPPED;
    return NULL;
  }

  return NULL;
}

test_st worker_TESTS[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"sanity", 0, sanity_TEST },
  {"gearman_worker_clone(NULL, NULL)", 0, gearman_worker_clone_NULL_NULL },
  {"gearman_worker_clone(NULL, source)", 0, gearman_worker_clone_NULL_SOURCE },
  {"gearman_worker_add_server(GEARMAN_GETADDRINFO)", false, gearman_worker_add_server_GEARMAN_GETADDRINFO_TEST },
  {"gearman_worker_add_server(GEARMAN_INVALID_ARGUMENT)", false, gearman_worker_add_server_GEARMAN_INVALID_ARGUMENT_TEST },
  {"echo", 0, echo_test },
  {"echo_multi", 0, echo_multi_test },
  {"options", 0, option_test },
  {"gearman_worker_add_function()", 0, gearman_worker_add_function_test },
  {"gearman_worker_add_function() multi", 0, gearman_worker_add_function_multi_test },
  {"gearman_worker_unregister_all()", 0, gearman_worker_unregister_all_test },
  {"gearman_worker_work() with timout", 0, gearman_worker_work_with_TEST },
  {"gearman_worker_work(GEARMAN_WORKER_NON_BLOCKING) with timout", 0, gearman_worker_work_with_GEARMAN_WORKER_NON_BLOCKING_TEST },
  {"gearman_worker_context", 0, gearman_worker_context_test },
  {"gearman_worker_failover", 0, gearman_worker_failover_test },
  {"gearman_worker_check_options(GEARMAN_WORKER_GRAB_UNIQ)", 0, gearman_worker_check_options_GEARMAN_WORKER_GRAB_UNIQ },
  {"gearman_worker_remove_options(GEARMAN_WORKER_GRAB_UNIQ)", 0, gearman_worker_remove_options_GEARMAN_WORKER_GRAB_UNIQ },
  {"gearman_worker_add_options(GEARMAN_WORKER_GRAB_UNIQ)", 0, gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ },
  {"gearman_worker_add_options(GEARMAN_WORKER_GRAB_UNIQ) worker_work()", 0, gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ_worker_work },
  {"gearman_worker_set_timeout(2) with failover", 0, gearman_worker_set_timeout_FAILOVER_TEST },
  {"gearman_return_t worker return coverage", 0, error_return_TEST },
  {"gearman_return_t GEARMAN_FAIL worker coverage", 0, GEARMAN_FAIL_return_TEST },
  {"gearman_return_t GEARMAN_ERROR worker coverage", 0, GEARMAN_ERROR_return_TEST },
  {"gearman_client_run_tasks()", 0, gearman_client_run_tasks_increase_TEST },
  {"gearman_client_run_tasks() GEARMAN_CLIENT_NON_BLOCKING", 0, gearman_client_run_tasks_increase_GEARMAN_CLIENT_NON_BLOCKING_TEST },
  {"gearman_client_run_tasks() chunked", 0, gearman_client_run_tasks_increase_chunk_TEST },
  {"gearman_client_job_status(is_known)", 0, gearman_client_job_status_is_known_TEST },
  {"echo_max", 0, echo_max_test },
  {"abandoned_worker", 0, abandoned_worker_test },
  {0, 0, 0}
};

test_st worker_defaults[] ={
  {"gearman_worker_timeout()", 0, gearman_worker_timeout_default_test },
  {0, 0, 0}
};

test_st gearman_worker_st_NULL_invocation_TESTS[] ={
  {"gearman_worker_free()", 0, gearman_worker_free_TEST },
  {"gearman_worker_error()", 0, gearman_worker_error_TEST },
  {"gearman_worker_error() no error", 0, gearman_worker_error_no_error_TEST },
  {"gearman_worker_errno()", 0, gearman_worker_errno_TEST },
  {"gearman_worker_errno() no error", 0, gearman_worker_errno_no_error_TEST },
  {"gearman_worker_options()", 0, gearman_worker_options_TEST },
  {0, 0, 0}
};

test_st gearman_worker_set_identifier_TESTS[] ={
  {"gearman_worker_set_identifier()", 0, gearman_worker_set_identifier_TEST },
  {0, 0, 0}
};

collection_st collection[] ={
  {"worker", 0, 0, worker_TESTS},
  {"worker defaults", 0, 0, worker_defaults},
  {"null gearman_worker_st invocation", 0, 0, gearman_worker_st_NULL_invocation_TESTS },
  {"gearman_worker_set_identifier()", 0, 0, gearman_worker_set_identifier_TESTS},
  {0, 0, 0, 0}
};

void get_world(libtest::Framework *world)
{
  world->collections(collection);
  world->create(world_create);
}
