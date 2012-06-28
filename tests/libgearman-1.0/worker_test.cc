/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <config.h>
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

#include <tests/client.h>
#include <tests/worker.h>
#include "tests/start_worker.h"
#include <tests/workers.h>

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

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

static test_return_t gearman_worker_clone_NULL_NULL(void *)
{
  gearman_worker_st *worker= gearman_worker_clone(NULL, NULL);

  test_truth(worker);
  test_compare(true, worker->options.allocated);

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_clone_NULL_SOURCE(void *)
{
  Worker source;

  gearman_worker_st *worker= gearman_worker_clone(NULL, &source);
  test_truth(worker);
  test_compare(true, worker->options.allocated);
  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_timeout_default_test(void *)
{
  Worker worker;

  test_compare(-1, gearman_worker_timeout(&worker));

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
  Worker worker;
  test_null(gearman_worker_error(&worker));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_errno_TEST(void *)
{
  test_compare(0, gearman_worker_errno(NULL));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_errno_no_error_TEST(void *)
{
  Worker worker;
  test_compare(0, gearman_worker_errno(&worker));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_options_TEST(void *)
{
  test_compare(gearman_worker_options_t(), gearman_worker_options(NULL));

  return TEST_SUCCESS;
}

static test_return_t option_test(void *)
{
  gearman_worker_st *gear;
  gearman_worker_options_t default_options;

  gear= gearman_worker_create(NULL);
  test_truth(gear);
  { // Initial Allocated, no changes
    test_truth(gear->options.allocated);
    test_false(gear->options.non_blocking);
    test_truth(gear->options.packet_init);
    test_false(gear->options.change);
    test_true(gear->options.grab_uniq);
    test_false(gear->options.timeout_return);
  }

  /* Set up for default options */
  default_options= gearman_worker_options(gear);

  /*
    We take the basic options, and push
    them back in. See if we change anything.
  */
  gearman_worker_set_options(gear, default_options);
  { // Initial Allocated, no changes
    test_truth(gear->options.allocated);
    test_false(gear->options.non_blocking);
    test_truth(gear->options.packet_init);
    test_false(gear->options.change);
    test_true(gear->options.grab_uniq);
    test_false(gear->options.timeout_return);
  }

  /*
    We will trying to modify non-mutable options (which should not be allowed)
  */
  {
    gearman_worker_remove_options(gear, GEARMAN_WORKER_ALLOCATED);
    { // Initial Allocated, no changes
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_true(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }

    gearman_worker_remove_options(gear, GEARMAN_WORKER_PACKET_INIT);
    { // Initial Allocated, no changes
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_true(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
  }

  /*
    We will test modifying GEARMAN_WORKER_NON_BLOCKING in several manners.
  */
  {
    gearman_worker_remove_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_true(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_add_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_true(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_set_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_set_options(gear, GEARMAN_WORKER_GRAB_UNIQ);
    { // Everything is now set to false except GEARMAN_WORKER_GRAB_UNIQ, and non-mutable options
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_truth(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    /*
      Reset options to default. Then add an option, and then add more options. Make sure
      the options are all additive.
    */
    {
      gearman_worker_set_options(gear, default_options);
      { // See if we return to defaults
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_true(gear->options.grab_uniq);
        test_false(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_true(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, (gearman_worker_options_t)(GEARMAN_WORKER_NON_BLOCKING|GEARMAN_WORKER_GRAB_UNIQ));
      { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
        test_truth(gear->options.allocated);
        test_truth(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_truth(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
    }
    /*
      Add an option, and then replace with that option plus a new option.
    */
    {
      gearman_worker_set_options(gear, default_options);
      { // See if we return to defaults
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_true(gear->options.grab_uniq);
        test_false(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_true(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, (gearman_worker_options_t)(GEARMAN_WORKER_TIMEOUT_RETURN|GEARMAN_WORKER_GRAB_UNIQ));
      { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_truth(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
    }
  }

  gearman_worker_free(gear);

  return TEST_SUCCESS;
}

static test_return_t echo_test(void*)
{
  Worker worker;

  test_compare(gearman_worker_echo(&worker, test_literal_param("This is my echo test")), GEARMAN_SUCCESS);

  return TEST_SUCCESS;
}

static test_return_t echo_multi_test(void *)
{
  Worker worker;

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
    test_compare(gearman_worker_echo(&worker, test_string_make_from_cstr(*ptr)),
                 GEARMAN_SUCCESS);
    ptr++;
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_server_GEARMAN_INVALID_ARGUMENT_TEST(void *)
{
  if (libtest::check_dns())
  {
    test_compare(GEARMAN_INVALID_ARGUMENT,
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
    test_compare(gearman_worker_add_server(worker, "nonexist.gearman.info", libtest::default_port()), GEARMAN_GETADDRINFO);
    gearman_worker_free(worker);
  }

  return TEST_SUCCESS;
}

static test_return_t echo_max_test(void *)
{
  Worker worker;

  gearman_worker_add_server(&worker, NULL, libtest::default_port());

  test_compare(GEARMAN_ARGUMENT_TOO_LARGE,
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
  // Sanity test on initial enum
  test_compare(0, int(GEARMAN_SUCCESS));
  test_compare(1, int(GEARMAN_IO_WAIT));

  gearman_client_st *client= gearman_client_create(NULL);
  test_true(client);
  test_compare(GEARMAN_SUCCESS, gearman_client_add_server(client, "localhost", libtest::default_port()));
  test_compare(GEARMAN_SUCCESS, gearman_client_echo(client, test_literal_param(__func__)));

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
    gearman_argument_t arg= gearman_argument_make(NULL, 0, (const char*)&x, sizeof(gearman_return_t));
    gearman_task_st *task= gearman_execute(client,
                                           test_literal_param(__func__),
                                           NULL, 0, // unique
                                           NULL, // gearman_task_attr_t
                                           &arg, // gearman_argument_t
                                           NULL); // context
    test_truth(task);

    gearman_return_t rc;
    bool is_known;
    do {
      rc= gearman_client_job_status(client, gearman_task_job_handle(task), &is_known, NULL, NULL, NULL);
    }  while (gearman_continue(rc) or is_known);

    gearman_result_st *result= gearman_task_result(task);
    test_false(result);
    {
      test_compare(gearman_task_return(task), GEARMAN_WORK_FAIL);
      test_false(handle->is_shutdown());
    }
  }

  // GEARMAN_SHUTDOWN is a special case
  {
    gearman_return_t x= GEARMAN_SHUTDOWN;
    gearman_argument_t arg= gearman_argument_make(NULL, 0, (const char*)&x, sizeof(gearman_return_t));
    gearman_task_st *task= gearman_execute(client,
                                           test_literal_param(__func__),
                                           NULL, 0, // unique
                                           NULL, // gearman_task_attr_t
                                           &arg, // gearman_argument_t
                                           NULL); // context
    test_truth(task);

    gearman_return_t rc;
    bool is_known;
    do {
      rc= gearman_client_job_status(client, gearman_task_job_handle(task), &is_known, NULL, NULL, NULL);
    }  while (gearman_continue(rc) or is_known);

    {
      test_compare(GEARMAN_SUCCESS, gearman_task_return(task));
    }
  }

  int client_timeout= gearman_client_timeout(client);
  // Test for unknown function
  {
    gearman_return_t x= GEARMAN_SUCCESS;
    gearman_argument_t arg= gearman_argument_make(NULL, 0, (const char*)&x, sizeof(gearman_return_t));
    gearman_client_set_timeout(client, 1500);
    gearman_task_st *task= gearman_execute(client,
                                           test_literal_param("unknown_function_to_fail_on"),
                                           NULL, 0, // unique
                                           NULL, // gearman_task_attr_t
                                           &arg, // gearman_argument_t
                                           NULL); // context
    test_true(task);
#if 0
    test_compare(gearman_task_return(task),
                 GEARMAN_SUCCESS);
#endif
  }
  
  // GEARMAN_SUCCESS after GEARMAN_SHUTDOWN should fail
  {
    gearman_return_t x= GEARMAN_SUCCESS;
    gearman_argument_t arg= gearman_argument_make(NULL, 0, (const char*)&x, sizeof(gearman_return_t));
    gearman_task_st *task= gearman_execute(client,
                                           test_literal_param(__func__),
                                           NULL, 0, // unique
                                           NULL, // gearman_task_attr_t
                                           &arg, // gearman_argument_t
                                           NULL); // context
    test_truth(task);

    gearman_return_t rc;
    bool is_known;
    do {
      rc= gearman_client_job_status(client, gearman_task_job_handle(task), &is_known, NULL, NULL, NULL);
    }  while (gearman_continue(rc) or is_known);

    {
      test_compare(gearman_task_return(task), GEARMAN_UNKNOWN_STATE);
    }
  }
  gearman_client_set_timeout(client, client_timeout);
  gearman_client_free(client);

  return TEST_SUCCESS;
}

static test_return_t GEARMAN_ERROR_return_TEST(void *)
{
  // Sanity test on initial enum
  test_compare(0, int(GEARMAN_SUCCESS));
  test_compare(1, int(GEARMAN_IO_WAIT));

  gearman_client_st *client= gearman_client_create(NULL);
  test_true(client);
  test_compare(GEARMAN_SUCCESS, gearman_client_add_server(client, "localhost", libtest::default_port()));
  test_compare(GEARMAN_SUCCESS, gearman_client_echo(client, test_literal_param(__func__)));

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
    gearman_task_st *task= gearman_execute(client,
                                           test_literal_param(__func__),
                                           NULL, 0, // unique
                                           NULL, // gearman_task_attr_t
                                           NULL, // gearman_argument_t
                                           NULL); // context
    test_truth(task);

    gearman_return_t rc;
    bool is_known;
    do {
      rc= gearman_client_job_status(client, gearman_task_job_handle(task), &is_known, NULL, NULL, NULL);
    }  while (gearman_continue(rc) or is_known);

    test_compare(gearman_task_return(task), GEARMAN_SUCCESS);
    test_zero(count); // Since we hit zero we know that we ran enough times.

    gearman_result_st *result= gearman_task_result(task);
    test_true(result);
    test_memcmp("OK", gearman_result_value(result), strlen("ok"));
  }

  gearman_client_free(client);

  return TEST_SUCCESS;
}

static test_return_t GEARMAN_FAIL_return_TEST(void *)
{
  // Sanity test on initial enum
  test_compare(0, int(GEARMAN_SUCCESS));
  test_compare(1, int(GEARMAN_IO_WAIT));

  gearman_client_st *client= gearman_client_create(NULL);
  test_true(client);
  test_compare(GEARMAN_SUCCESS, gearman_client_add_server(client, "localhost", libtest::default_port()));
  test_compare(GEARMAN_SUCCESS, gearman_client_echo(client, test_literal_param(__func__)));

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
    gearman_task_st *task= gearman_execute(client,
                                           test_literal_param(__func__),
                                           NULL, 0, // unique
                                           NULL, // gearman_task_attr_t
                                           &arg, // gearman_argument_t
                                           NULL); // context
    test_truth(task);

    gearman_return_t rc;
    bool is_known;
    do {
      rc= gearman_client_job_status(client, gearman_task_job_handle(task), &is_known, NULL, NULL, NULL);
    }  while (gearman_continue(rc) or is_known);

    {
      test_compare(GEARMAN_FAIL, gearman_task_return(task));
    }
  }

  gearman_client_free(client);

  return TEST_SUCCESS;
}

static test_return_t abandoned_worker_test(void *)
{
  gearman_job_handle_t job_handle;
  const void *args[2];
  size_t args_size[2];

  {
    Client client;
    gearman_client_add_server(&client, NULL, libtest::default_port());
    test_compare(gearman_client_do_background(&client, "abandoned_worker", NULL, NULL, 0, job_handle),
                 GEARMAN_SUCCESS);
  }

  /* Now take job with one worker. */
  gearman_universal_st universal;
  gearman_universal_initialize(universal);

  gearman_connection_st *worker1;
  test_truth(worker1= gearman_connection_create(universal, NULL));

  worker1->set_host(NULL, libtest::default_port());

  gearman_packet_st packet;
  args[0]= "abandoned_worker";
  args_size[0]= strlen("abandoned_worker");
  test_compare(GEARMAN_SUCCESS,
               gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                          GEARMAN_COMMAND_CAN_DO,
                                          args, args_size, 1));

  test_compare(worker1->send_packet(packet, true),
               GEARMAN_SUCCESS);

  gearman_packet_free(&packet);

  gearman_return_t ret;
  test_compare(GEARMAN_SUCCESS,
               gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST, GEARMAN_COMMAND_GRAB_JOB, NULL, NULL, 0));

  test_compare(GEARMAN_SUCCESS, worker1->send_packet(packet, true));

  gearman_packet_free(&packet);

  worker1->receiving(packet, ret, false);
  test_truth(not (ret != GEARMAN_SUCCESS or packet.command != GEARMAN_COMMAND_JOB_ASSIGN));

  test_strcmp(job_handle, packet.arg[0]); // unexepcted job

  gearman_packet_free(&packet);

  gearman_connection_st *worker2;
  test_truth(worker2= gearman_connection_create(universal, NULL));

  worker2->set_host(NULL, libtest::default_port());

  args[0]= "abandoned_worker";
  args_size[0]= strlen("abandoned_worker");
  test_compare(GEARMAN_SUCCESS, gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                           GEARMAN_COMMAND_CAN_DO,
                                                           args, args_size, 1));

  test_compare(GEARMAN_SUCCESS, worker2->send_packet(packet, true));

  gearman_packet_free(&packet);

  args[0]= job_handle;
  args_size[0]= strlen(job_handle) + 1;
  args[1]= "test";
  args_size[1]= 4;
  test_compare(GEARMAN_SUCCESS, gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                           GEARMAN_COMMAND_WORK_COMPLETE,
                                                           args, args_size, 2));

  test_compare(GEARMAN_SUCCESS, worker2->send_packet(packet, true));

  gearman_packet_free(&packet);

  gearman_universal_set_timeout(universal, 1000);
  worker2->receiving(packet, ret, false);
  test_truth(not (ret != GEARMAN_SUCCESS or packet.command != GEARMAN_COMMAND_ERROR));

  delete worker1;
  delete worker2;
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
  Worker worker;

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_function(&worker, function_name,0, fail_worker, NULL));

  test_compare(true, gearman_worker_function_exist(&worker, test_string_make_from_array(function_name)));

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_unregister(&worker, function_name));

  test_compare(false, gearman_worker_function_exist(&worker, function_name, strlen(function_name)));

  /* Make sure we have removed it */
  test_compare(GEARMAN_NO_REGISTERED_FUNCTION, 
               gearman_worker_unregister(&worker, function_name));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_function_multi_test(void *)
{
  Worker worker;

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];
    snprintf(buffer, 1024, "%u%s", x, __func__);

    test_compare(GEARMAN_SUCCESS,
                 gearman_worker_add_function(&worker, buffer, 0, fail_worker, NULL));
  }

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, 1024, "%u%s", x, __func__);
    test_compare(GEARMAN_SUCCESS,
                 gearman_worker_unregister(&worker, buffer));
  }

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, 1024, "%u%s", x, __func__);
    test_compare(GEARMAN_NO_REGISTERED_FUNCTION,
                 gearman_worker_unregister(&worker, buffer));
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_unregister_all_test(void *)
{
  Worker worker;
  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%u%s", x, __func__);
    gearman_return_t rc= gearman_worker_add_function(&worker,
						     buffer,
						     0, fail_worker, NULL);

    test_compare(rc, GEARMAN_SUCCESS);
  }

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_unregister_all(&worker));

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, sizeof(buffer), "%u%s", x, __func__);
    gearman_return_t rc= gearman_worker_unregister(&worker, buffer);
    test_compare(rc, GEARMAN_NO_REGISTERED_FUNCTION);
  }

  test_compare(gearman_worker_unregister_all(&worker),
               GEARMAN_NO_REGISTERED_FUNCTIONS);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_work_with_test(void *)
{
  Worker worker;

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  test_compare(gearman_worker_add_function(&worker,
                                           function_name,
                                           0, fail_worker, NULL),
               GEARMAN_SUCCESS);

  gearman_worker_set_timeout(&worker, 0);

  test_compare(GEARMAN_TIMEOUT,
               gearman_worker_work(&worker));

  test_compare(GEARMAN_TIMEOUT,
               gearman_worker_work(&worker));

  /* Make sure we have removed the worker function */
  test_compare(GEARMAN_SUCCESS,
               gearman_worker_unregister(&worker, function_name));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_context_test(void *)
{
  Worker worker;

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
  Worker worker;

  test_true(worker->options.grab_uniq);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_remove_options_GEARMAN_WORKER_GRAB_UNIQ(void *)
{
  Worker worker;

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  char unique_name[GEARMAN_MAX_UNIQUE_SIZE];
  snprintf(unique_name, GEARMAN_MAX_UNIQUE_SIZE, "_%s%d", __func__, int(random())); 

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_server(&worker, NULL, libtest::default_port()));

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_function(&worker, function_name, 0, no_unique_worker, NULL));

  {
    Client client;
    test_compare(GEARMAN_SUCCESS,
                 gearman_client_add_server(&client, NULL, libtest::default_port()));
    test_compare(gearman_client_do_background(&client, function_name, unique_name,
                                              test_string_make_from_array(unique_name), NULL),
                 GEARMAN_SUCCESS);
  }

  gearman_worker_remove_options(&worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_false(worker->options.grab_uniq);

  gearman_worker_set_timeout(&worker, 800);

  gearman_return_t rc;
  gearman_job_st *job= gearman_worker_grab_job(&worker, NULL, &rc);
  test_compare(rc, GEARMAN_SUCCESS);
  test_truth(job);

  size_t size= 0;
  void *result= no_unique_worker(job, NULL, &size, &rc);
  test_compare(rc, GEARMAN_SUCCESS);
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
    Client client;
    test_compare(GEARMAN_SUCCESS,
                 gearman_client_add_server(&client, NULL, libtest::default_port()));

    test_compare(gearman_client_do_background(&client, function_name, unique_name,
                                              test_string_make_from_array(unique_name), NULL), 
                 GEARMAN_SUCCESS);
  }

  Worker worker;

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_server(&worker, NULL, libtest::default_port()));

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_function(&worker, function_name, 0, check_unique_worker, NULL));

  gearman_worker_add_options(&worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_true(worker->options.grab_uniq);

  gearman_return_t rc;
  gearman_job_st *job= gearman_worker_grab_job(&worker, NULL, &rc);
  test_compare(GEARMAN_SUCCESS, rc);
  test_truth(job);

  size_t size= 0;
  void *result= check_unique_worker(job, NULL, &size, &rc);
  test_compare(GEARMAN_SUCCESS, rc);
  test_truth(result);
  test_truth(size);
  free(result);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_set_identifier_TEST(void *)
{
  Worker worker;

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_server(&worker, NULL, libtest::default_port()));

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_set_identifier(&worker, test_literal_param(__func__)));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ_worker_work(void *)
{
  Worker worker;

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  char unique_name[GEARMAN_MAX_UNIQUE_SIZE];
  snprintf(unique_name, GEARMAN_MAX_UNIQUE_SIZE, "_%s%d", __func__, int(random())); 

  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_server(&worker, NULL, libtest::default_port()));

  bool success= false;
  test_compare(GEARMAN_SUCCESS,
               gearman_worker_add_function(&worker, function_name, 0, check_unique_worker, &success));

  {
    Client client;
    test_compare(GEARMAN_SUCCESS,
                 gearman_client_add_server(&client, NULL, libtest::default_port()));
    test_compare(gearman_client_do_background(&client, function_name, unique_name,
                                              test_string_make_from_array(unique_name), NULL),
                 GEARMAN_SUCCESS);
  }

  test_true(worker->options.grab_uniq);
  gearman_worker_add_options(&worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_truth(worker->options.grab_uniq);

  gearman_worker_set_timeout(&worker, 400);
  test_compare(gearman_worker_work(&worker), GEARMAN_SUCCESS);

  test_truth(success);


  return TEST_SUCCESS;
}

static test_return_t _increase_TEST(gearman_function_t &func, gearman_client_options_t options, size_t block_size)
{
  Client client;
  test_compare(GEARMAN_SUCCESS,
               gearman_client_add_server(&client, NULL, libtest::default_port()));

  test_compare(GEARMAN_SUCCESS, gearman_client_echo(&client, test_literal_param(__func__)));

  gearman_client_add_options(&client, options);

  std::auto_ptr<worker_handle_st> handle(test_worker_start(libtest::default_port(),
                                                           NULL,
                                                           __func__,
                                                           func,
                                                           NULL,
                                                           gearman_worker_options_t(),
                                                           0)); // timeout

  for (size_t x= 1; x < 24; x++)
  {
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

    test_compare(GEARMAN_SUCCESS,
                 gearman_task_return(task));

    gearman_result_st *result= gearman_task_result(task);
    test_true(result);
    test_compare(gearman_result_size(result), workload.size());
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
  Worker worker;

  test_compare(GEARMAN_SUCCESS, gearman_worker_add_server(&worker, NULL, libtest::default_port()));
  test_compare(GEARMAN_SUCCESS, gearman_worker_add_server(&worker, NULL, libtest::default_port() +1));

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  test_compare(GEARMAN_SUCCESS, 
               gearman_worker_add_function(&worker, function_name, 0, fail_worker, NULL));

  gearman_worker_set_timeout(&worker, 400);

  test_compare(GEARMAN_TIMEOUT, gearman_worker_work(&worker));

  /* Make sure we have remove worker function */
  test_compare(GEARMAN_SUCCESS,
               gearman_worker_unregister(&worker, function_name));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_set_timeout_FAILOVER_TEST(void *)
{
  test_skip_valgrind(); // lp:961904

  Worker worker;

  test_compare(GEARMAN_SUCCESS, gearman_worker_add_server(&worker, NULL, libtest::default_port()));
  in_port_t non_exist_server= libtest::default_port();
  test_compare(GEARMAN_SUCCESS, gearman_worker_add_server(&worker, NULL, non_exist_server));

  char function_name[GEARMAN_FUNCTION_MAX_SIZE];
  snprintf(function_name, GEARMAN_FUNCTION_MAX_SIZE, "_%s%d", __func__, int(random())); 

  test_compare(GEARMAN_SUCCESS, 
               gearman_worker_add_function(&worker, function_name, 0, fail_worker, NULL));

  gearman_worker_set_timeout(&worker, 2);

  test_compare(GEARMAN_TIMEOUT, gearman_worker_work(&worker));

  /* Make sure we have remove worker function */
  test_compare(GEARMAN_SUCCESS,
               gearman_worker_unregister(&worker, function_name));

  return TEST_SUCCESS;
}

/*********************** World functions **************************************/

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (server_startup(servers, "gearmand", libtest::default_port(), 0, NULL) == false)
  {
    error= TEST_FAILURE;
    return NULL;
  }

  return NULL;
}

test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
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
  {"gearman_worker_work() with timout", 0, gearman_worker_work_with_test },
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
  {"worker", 0, 0, tests},
  {"worker defaults", 0, 0, worker_defaults},
  {"null gearman_worker_st invocation", 0, 0, gearman_worker_st_NULL_invocation_TESTS },
  {"gearman_worker_set_identifier()", 0, 0, gearman_worker_set_identifier_TESTS},
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  world->collections(collection);
  world->create(world_create);
}
