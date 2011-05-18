/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#if defined(NDEBUG)
# undef NDEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GEARMAN_CORE

#include <libgearman/gearman.h>
#include <libgearman/connection.hpp>
#include "libgearman/packet.hpp"
#include "libgearman/universal.hpp"

#include "libtest/test.h"
#include "libtest/server.h"

#define WORKER_TEST_PORT 32125

struct worker_test_st
{
  pid_t gearmand_pid;
  gearman_worker_st *_worker;

  gearman_worker_st *worker()
  {
    return _worker;
  }

  worker_test_st() :
    gearmand_pid(-1)
  {
    _worker= gearman_worker_create(NULL);
  }

  ~worker_test_st()
  {
    gearman_worker_free(_worker);
  }
};

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
#endif

static test_return_t clone_test(void *object)
{
  gearman_worker_st *from= (gearman_worker_st *)object;
  gearman_worker_st *worker;

  test_truth(worker= gearman_worker_clone(NULL, NULL));

  test_truth(worker->options.allocated);

  gearman_worker_free(worker);

  test_truth(worker= gearman_worker_clone(NULL, from));

  gearman_worker_free(worker);

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
    test_false(gear->options.grab_uniq);
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
    test_false(gear->options.grab_uniq);
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
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }

    gearman_worker_remove_options(gear, GEARMAN_WORKER_PACKET_INIT);
    { // Initial Allocated, no changes
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
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
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_add_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
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
        test_false(gear->options.grab_uniq);
        test_false(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_false(gear->options.grab_uniq);
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
        test_false(gear->options.grab_uniq);
        test_false(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.change);
        test_false(gear->options.grab_uniq);
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

static test_return_t echo_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;

  gearman_return_t rc;
  test_true_got(gearman_success(rc= gearman_worker_echo(worker, gearman_literal_param("This is my echo test"))), gearman_strerror(rc));

  return TEST_SUCCESS;
}

static test_return_t echo_multi_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
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
    gearman_return_t rc;
    test_true_got(gearman_success(rc= gearman_worker_echo(worker, gearman_c_str_param(*ptr))), gearman_strerror(rc));
    ptr++;
  }

  return TEST_SUCCESS;
}

static test_return_t echo_max_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;

  gearman_return_t rc= gearman_worker_echo(worker, "This is my echo test", SIZE_MAX);
  test_truth(rc == GEARMAN_ARGUMENT_TOO_LARGE);

  return TEST_SUCCESS;
}

static test_return_t abandoned_worker_test(void *)
{
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  gearman_packet_st packet;
  const void *args[2];
  size_t args_size[2];

  {
    gearman_client_st *client;
    client= gearman_client_create(NULL);
    test_truth(client);
    gearman_client_add_server(client, NULL, WORKER_TEST_PORT);
    test_true_got(gearman_success(gearman_client_do_background(client, "abandoned_worker", NULL, NULL, 0, job_handle)), gearman_client_error(client));
    gearman_client_free(client);
  }

  /* Now take job with one worker. */
  gearman_universal_st universal;
  gearman_universal_initialize(universal);

  gearman_connection_st *worker1;
  test_truth(worker1= gearman_connection_create(universal, NULL, NULL));

  gearman_connection_set_host(worker1, NULL, WORKER_TEST_PORT);

  args[0]= "abandoned_worker";
  args_size[0]= strlen("abandoned_worker");
  test_truth(gearman_success(gearman_packet_create_args(universal, &packet, GEARMAN_MAGIC_REQUEST,
							GEARMAN_COMMAND_CAN_DO,
							args, args_size, 1)));

  gearman_return_t ret;
  test_true_got(gearman_success(ret= gearman_connection_send(worker1, &packet, true)), gearman_strerror(ret));

  gearman_packet_free(&packet);

  test_true_got(gearman_success(ret= gearman_packet_create_args(universal, &packet, GEARMAN_MAGIC_REQUEST,
                                                                GEARMAN_COMMAND_GRAB_JOB,
                                                                NULL, NULL, 0)), gearman_strerror(ret));

  test_true_got(gearman_success(ret= gearman_connection_send(worker1, &packet, true)), gearman_strerror(ret));

  gearman_packet_free(&packet);

  gearman_connection_recv(worker1, &packet, &ret, false);
  test_truth(not (ret != GEARMAN_SUCCESS || packet.command != GEARMAN_COMMAND_JOB_ASSIGN));

  test_strcmp(job_handle, packet.arg[0]); // unexepcted job

  gearman_packet_free(&packet);

  gearman_connection_st *worker2;
  test_truth(worker2= gearman_connection_create(universal, NULL, NULL));

  gearman_connection_set_host(worker2, NULL, WORKER_TEST_PORT);

  args[0]= "abandoned_worker";
  args_size[0]= strlen("abandoned_worker");
  test_true_got(gearman_success(ret= gearman_packet_create_args(universal, &packet, GEARMAN_MAGIC_REQUEST,
								GEARMAN_COMMAND_CAN_DO,
								args, args_size, 1)), gearman_strerror(ret));

  test_true_got(gearman_success(ret= gearman_connection_send(worker2, &packet, true)), gearman_strerror(ret));

  gearman_packet_free(&packet);

  args[0]= job_handle;
  args_size[0]= strlen(job_handle) + 1;
  args[1]= "test";
  args_size[1]= 4;
  test_true_got(gearman_success(ret= gearman_packet_create_args(universal, &packet, GEARMAN_MAGIC_REQUEST,
								GEARMAN_COMMAND_WORK_COMPLETE,
								args, args_size, 2)), gearman_strerror(ret));

  test_true_got(gearman_success(ret= gearman_connection_send(worker2, &packet, true)), gearman_strerror(ret));

  gearman_packet_free(&packet);

  gearman_universal_set_timeout(universal, 1000);
  gearman_connection_recv(worker2, &packet, &ret, false);
  test_truth(not (ret != GEARMAN_SUCCESS || packet.command != GEARMAN_COMMAND_ERROR));

  gearman_connection_free(worker1);
  gearman_connection_free(worker2);
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

static test_return_t gearman_worker_add_function_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  const char *function_name= "fail_worker";

  gearman_return_t rc;
  rc= gearman_worker_add_function(worker,
				  function_name,
				  0, fail_worker, NULL);
  test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));

  test_truth(gearman_worker_function_exist(worker, gearman_c_str_param(function_name)));

  test_true_got(gearman_success(rc= gearman_worker_unregister(worker, function_name)), gearman_strerror(rc));

  bool found;
  found= gearman_worker_function_exist(worker, function_name, strlen(function_name));
  test_false(found);

  /* Make sure we have removed it */
  rc= gearman_worker_unregister(worker, function_name);
  test_true_got(rc == GEARMAN_NO_REGISTERED_FUNCTION, gearman_strerror(rc));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_function_multi_test(void *object)
{
  gearman_return_t rc;
  gearman_worker_st *worker= (gearman_worker_st *)object;
  const char *function_name_ext= "fail_worker";

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];
    snprintf(buffer, 1024, "%u%s", x, function_name_ext);
    rc= gearman_worker_add_function(worker,
                                    buffer,
                                    0, fail_worker, NULL);

    test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));
  }

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, 1024, "%u%s", x, function_name_ext);
    rc= gearman_worker_unregister(worker, buffer);
    test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));
  }

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, 1024, "%u%s", x, function_name_ext);
    rc= gearman_worker_unregister(worker, buffer);
    test_true_got(rc == GEARMAN_NO_REGISTERED_FUNCTION, gearman_strerror(rc));
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_unregister_all_test(void *object)
{
  gearman_return_t rc;
  gearman_worker_st *worker= (gearman_worker_st *)object;
  const char *function_name_ext= "fail_worker";

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%u%s", x, function_name_ext);
    rc= gearman_worker_add_function(worker,
                                    buffer,
                                    0, fail_worker, NULL);

    test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));
  }

  rc= gearman_worker_unregister_all(worker);

  for (uint32_t x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, sizeof(buffer), "%u%s", x, function_name_ext);
    rc= gearman_worker_unregister(worker, buffer);
    test_true_got(rc == GEARMAN_NO_REGISTERED_FUNCTION, gearman_strerror(rc));
  }

  rc= gearman_worker_unregister_all(worker);
  test_true_got(rc == GEARMAN_NO_REGISTERED_FUNCTIONS, gearman_strerror(rc));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_work_with_test(void *object)
{
  gearman_return_t rc;
  gearman_worker_st *worker= (gearman_worker_st *)object;
  const char *function_name= "fail_worker";

  rc= gearman_worker_add_function(worker,
				  function_name,
				  0, fail_worker, NULL);
  test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));

  gearman_worker_set_timeout(worker, 2);

  rc= gearman_worker_work(worker);
  test_true_got(rc == GEARMAN_TIMEOUT, gearman_strerror(rc));

  /* Make sure we have remove worker function */
  rc= gearman_worker_unregister(worker, function_name);
  test_true_got(rc == GEARMAN_SUCCESS, gearman_strerror(rc));

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_context_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  test_truth(worker);

  test_false(gearman_worker_context(worker));

  int value= 5;
  gearman_worker_set_context(worker, &value);

  int *ptr= (int *)gearman_worker_context(worker);

  test_truth(ptr == &value);
  test_truth(*ptr == value);
  gearman_worker_set_context(worker, NULL);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_remove_options_GEARMAN_WORKER_GRAB_UNIQ(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;

  const char *function_name= "_test_worker";
  const char *unique_name= "fooman";
  gearman_return_t rc;
  test_true_got(gearman_success(rc= gearman_worker_add_function(worker,
                                                                function_name,
                                                                0, 
                                                                no_unique_worker, NULL)), gearman_strerror(rc));

  {
    gearman_client_st *client;
    test_truth(client= gearman_client_create(NULL));
    gearman_client_add_server(client, NULL, WORKER_TEST_PORT);
    test_true_got(gearman_success(gearman_client_do_background(client, function_name, unique_name, gearman_c_str_param(unique_name), NULL)), gearman_client_error(client));
    gearman_client_free(client);
  }

  test_false(worker->options.grab_uniq);
  gearman_worker_add_options(worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_truth(worker->options.grab_uniq);

  gearman_worker_remove_options(worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_false(worker->options.grab_uniq);

  gearman_job_st *job= gearman_worker_grab_job(worker, NULL, &rc);
  test_true_got(gearman_success(rc), gearman_strerror(rc));
  test_truth(job);
  size_t size= 0;
  void *result= no_unique_worker(job, NULL, &size, &rc);
  test_true_got(gearman_success(rc), gearman_strerror(rc));
  test_false(result);
  test_false(size);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;

  const char *function_name= "_test_worker";
  const char *unique_name= "fooman";
  gearman_return_t rc;
  test_true_got(gearman_success(rc= gearman_worker_add_function(worker,
                                                                function_name,
                                                                0, 
                                                                check_unique_worker, NULL)), gearman_strerror(rc));

  {
    gearman_client_st *client;
    test_truth(client= gearman_client_create(NULL));
    gearman_client_add_server(client, NULL, WORKER_TEST_PORT);
    test_true_got(gearman_success(gearman_client_do_background(client, function_name, unique_name, gearman_c_str_param(unique_name), NULL)), gearman_client_error(client));
    gearman_client_free(client);
  }

  test_false(worker->options.grab_uniq);
  gearman_worker_add_options(worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_truth(worker->options.grab_uniq);
  gearman_job_st *job= gearman_worker_grab_job(worker, NULL, &rc);
  test_true_got(gearman_success(rc), gearman_strerror(rc));
  test_truth(job);
  size_t size= 0;
  void *result= check_unique_worker(job, NULL, &size, &rc);
  test_true_got(gearman_success(rc), gearman_strerror(rc));
  test_truth(result);
  test_truth(size);
  free(result);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ_worker_work(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;

  const char *function_name= "_test_worker";
  const char *unique_name= "fooman";
  gearman_return_t rc;
  bool success= false;
  test_true_got(gearman_success(rc= gearman_worker_add_function(worker,
                                                                function_name,
                                                                0, 
                                                                check_unique_worker, &success)), gearman_strerror(rc));

  {
    gearman_client_st *client;
    test_truth(client= gearman_client_create(NULL));
    gearman_client_add_server(client, NULL, WORKER_TEST_PORT);
    test_true_got(gearman_success(gearman_client_do_background(client, function_name, unique_name, gearman_c_str_param(unique_name), NULL)), gearman_client_error(client));
    gearman_client_free(client);
  }

  test_false(worker->options.grab_uniq);
  gearman_worker_add_options(worker, GEARMAN_WORKER_GRAB_UNIQ);
  test_truth(worker->options.grab_uniq);

  test_true_got(gearman_success(rc= gearman_worker_work(worker)), gearman_strerror(rc));

  test_truth(success);


  return TEST_SUCCESS;
}

static test_return_t gearman_worker_failover_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  gearman_worker_st *cloned;
  gearman_return_t rc;

  cloned= gearman_worker_clone(NULL, worker);
  test_true_got(gearman_success(rc= gearman_worker_add_server(cloned, NULL, WORKER_TEST_PORT)), gearman_strerror(rc));
  test_true_got(gearman_success(rc= gearman_worker_add_server(cloned, NULL, WORKER_TEST_PORT +1)), gearman_strerror(rc));

  const char *function_name= "fail_worker";
  test_true_got(gearman_success(rc= gearman_worker_add_function(cloned,
				  function_name,
				  0, fail_worker, NULL)), gearman_strerror(rc));

  gearman_worker_set_timeout(cloned, 2);

  rc= gearman_worker_work(cloned);
  test_true_got(rc == GEARMAN_TIMEOUT, gearman_strerror(rc));

  /* Make sure we have remove worker function */
  test_true_got(gearman_success(rc= gearman_worker_unregister(cloned, function_name)) , gearman_strerror(rc));

  gearman_worker_free(cloned);

  return TEST_SUCCESS;
}

/*********************** World functions **************************************/

static void *world_create(test_return_t *error)
{
  pid_t gearmand_pid;

  gearmand_pid= test_gearmand_start(WORKER_TEST_PORT, 0, NULL);
  if (gearmand_pid == -1)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  worker_test_st *test= new worker_test_st;
  if (not test)
  {
    *error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  test->gearmand_pid= gearmand_pid;

  if (gearman_worker_add_server(test->worker(), NULL, WORKER_TEST_PORT) != GEARMAN_SUCCESS)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  *error= TEST_SUCCESS;

  return (void *)test;
}

static test_return_t world_destroy(void *object)
{
  worker_test_st *test= (worker_test_st *)object;

  test_gearmand_stop(test->gearmand_pid);

  delete test;

  return TEST_SUCCESS;
}

test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"clone", 0, clone_test },
  {"echo", 0, echo_test },
  {"echo_multi", 0, echo_multi_test },
  {"options", 0, option_test },
  {"gearman_worker_add_function()", 0, gearman_worker_add_function_test },
  {"gearman_worker_add_function() multi", 0, gearman_worker_add_function_multi_test },
  {"gearman_worker_unregister_all()", 0, gearman_worker_unregister_all_test },
  {"gearman_worker_work() with timout", 0, gearman_worker_work_with_test },
  {"gearman_worker_context", 0, gearman_worker_context_test },
  {"gearman_worker_failover", 0, gearman_worker_failover_test },
  {"gearman_worker_remove_options(GEARMAN_WORKER_GRAB_UNIQ)", 0, gearman_worker_remove_options_GEARMAN_WORKER_GRAB_UNIQ },
  {"gearman_worker_add_options(GEARMAN_WORKER_GRAB_UNIQ)", 0, gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ },
  {"gearman_worker_add_options(GEARMAN_WORKER_GRAB_UNIQ) worker_work()", 0, gearman_worker_add_options_GEARMAN_WORKER_GRAB_UNIQ_worker_work },
  {"echo_max", 0, echo_max_test },
  {"abandoned_worker", 0, abandoned_worker_test },
  {0, 0, 0}
};

collection_st collection[] ={
  {"worker", 0, 0, tests},
  {0, 0, 0, 0}
};


typedef test_return_t (*libgearman_test_prepost_callback_fn)(worker_test_st *);
typedef test_return_t (*libgearman_test_callback_fn)(gearman_worker_st *);
static test_return_t _runner_prepost_default(libgearman_test_prepost_callback_fn func, worker_test_st *container)
{
  if (func)
  {
    return func(container);
  }

  return TEST_SUCCESS;
}

static test_return_t _runner_default(libgearman_test_callback_fn func, worker_test_st *container)
{
  if (func)
  {
    test_return_t rc;

    if (container->worker())
    {
      gearman_worker_st *worker= gearman_worker_clone(NULL, container->worker());
      test_truth(worker);
      rc= func(worker);
      gearman_worker_free(worker);
    }
    else
    {
      rc= func(container->worker());
    }

    return rc;
  }

  return TEST_SUCCESS;
}


static world_runner_st runner= {
  (test_callback_runner_fn)_runner_prepost_default,
  (test_callback_runner_fn)_runner_default,
  (test_callback_runner_fn)_runner_prepost_default
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
  world->runner= &runner;
}
