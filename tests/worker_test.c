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

#include "test.h"
#include "test_gearmand.h"

#define WORKER_TEST_PORT 32123

typedef struct
{
  pid_t gearmand_pid;
  gearman_worker_st worker;
} worker_test_st;

/* Prototypes */
test_return_t init_test(void *object);
test_return_t allocation_test(void *object);
test_return_t clone_test(void *object);
test_return_t option_test(void *object);

void *create(void *object);
void destroy(void *object);
test_return_t pre(void *object);
test_return_t post(void *object);
test_return_t flush(void);

void *world_create(test_return_t *error);
test_return_t world_destroy(void *object);

test_return_t init_test(void *object __attribute__((unused)))
{
  gearman_worker_st worker;

  if (gearman_worker_create(&worker) == NULL)
    return TEST_FAILURE;

  gearman_worker_free(&worker);

  return TEST_SUCCESS;
}

test_return_t allocation_test(void *object __attribute__((unused)))
{
  gearman_worker_st *worker;

  worker= gearman_worker_create(NULL);
  if (worker == NULL)
    return TEST_FAILURE;

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

test_return_t clone_test(void *object)
{
  gearman_worker_st *from= (gearman_worker_st *)object;
  gearman_worker_st *worker;

  worker= gearman_worker_clone(NULL, NULL);

  test_truth(worker);
  test_truth(worker->options.allocated);

  gearman_worker_free(worker);

  worker= gearman_worker_clone(NULL, from);
  if (worker == NULL)
    return TEST_FAILURE;

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

test_return_t option_test(void *object __attribute__((unused)))
{
  gearman_worker_st *gear;
  gearman_worker_options_t default_options;

  gear= gearman_worker_create(NULL);
  test_truth(gear);
  { // Initial Allocated, no changes
    test_truth(gear->options.allocated);
    test_false(gear->options.non_blocking);
    test_truth(gear->options.packet_init);
    test_false(gear->options.grab_job_in_use);
    test_false(gear->options.pre_sleep_in_use);
    test_false(gear->options.work_job_in_use);
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
    test_false(gear->options.grab_job_in_use);
    test_false(gear->options.pre_sleep_in_use);
    test_false(gear->options.work_job_in_use);
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
      test_false(gear->options.grab_job_in_use);
      test_false(gear->options.pre_sleep_in_use);
      test_false(gear->options.work_job_in_use);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_remove_options(gear, GEARMAN_WORKER_PACKET_INIT);
    { // Initial Allocated, no changes
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.grab_job_in_use);
      test_false(gear->options.pre_sleep_in_use);
      test_false(gear->options.work_job_in_use);
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
      test_false(gear->options.grab_job_in_use);
      test_false(gear->options.pre_sleep_in_use);
      test_false(gear->options.work_job_in_use);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_add_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.grab_job_in_use);
      test_false(gear->options.pre_sleep_in_use);
      test_false(gear->options.work_job_in_use);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_set_options(gear, GEARMAN_WORKER_NON_BLOCKING);
    { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.grab_job_in_use);
      test_false(gear->options.pre_sleep_in_use);
      test_false(gear->options.work_job_in_use);
      test_false(gear->options.change);
      test_false(gear->options.grab_uniq);
      test_false(gear->options.timeout_return);
    }
    gearman_worker_set_options(gear, GEARMAN_WORKER_GRAB_UNIQ);
    { // Everything is now set to false except GEARMAN_WORKER_GRAB_UNIQ, and non-mutable options
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_truth(gear->options.packet_init);
      test_false(gear->options.grab_job_in_use);
      test_false(gear->options.pre_sleep_in_use);
      test_false(gear->options.work_job_in_use);
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
        test_false(gear->options.grab_job_in_use);
        test_false(gear->options.pre_sleep_in_use);
        test_false(gear->options.work_job_in_use);
        test_false(gear->options.change);
        test_false(gear->options.grab_uniq);
        test_false(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.grab_job_in_use);
        test_false(gear->options.pre_sleep_in_use);
        test_false(gear->options.work_job_in_use);
        test_false(gear->options.change);
        test_false(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_NON_BLOCKING|GEARMAN_WORKER_GRAB_UNIQ);
      { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
        test_truth(gear->options.allocated);
        test_truth(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.grab_job_in_use);
        test_false(gear->options.pre_sleep_in_use);
        test_false(gear->options.work_job_in_use);
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
        test_false(gear->options.grab_job_in_use);
        test_false(gear->options.pre_sleep_in_use);
        test_false(gear->options.work_job_in_use);
        test_false(gear->options.change);
        test_false(gear->options.grab_uniq);
        test_false(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.grab_job_in_use);
        test_false(gear->options.pre_sleep_in_use);
        test_false(gear->options.work_job_in_use);
        test_false(gear->options.change);
        test_false(gear->options.grab_uniq);
        test_truth(gear->options.timeout_return);
      }
      gearman_worker_add_options(gear, GEARMAN_WORKER_TIMEOUT_RETURN|GEARMAN_WORKER_GRAB_UNIQ);
      { // GEARMAN_WORKER_NON_BLOCKING set to default, by default.
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_truth(gear->options.packet_init);
        test_false(gear->options.grab_job_in_use);
        test_false(gear->options.pre_sleep_in_use);
        test_false(gear->options.work_job_in_use);
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
  size_t value_length;
  const char *value= "This is my echo test";

  value_length= strlen(value);

  rc= gearman_worker_echo(worker, value, value_length);
  test_truth(rc == GEARMAN_SUCCESS);

  return TEST_SUCCESS;
}

static test_return_t echo_multi_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  gearman_return_t rc;
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
    rc= gearman_worker_echo(worker, value, strlen(*ptr));
    test_truth( rc == GEARMAN_SUCCESS);
    ptr++;
  }

  return TEST_SUCCESS;
}

static test_return_t echo_max_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  gearman_return_t rc;
  const char *value= "This is my echo test";

  rc= gearman_worker_echo(worker, value, SIZE_MAX);
  test_truth(rc == GEARMAN_ARGUMENT_TOO_LARGE);

  return TEST_SUCCESS;
}

static test_return_t abandoned_worker_test(void *object __attribute__((unused)))
{
  gearman_client_st client;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  gearman_return_t ret;
  gearman_universal_st gearman;
  gearman_connection_st worker1;
  gearman_connection_st worker2;
  gearman_packet_st packet;
  const void *args[2];
  size_t args_size[2];

  assert(gearman_client_create(&client) != NULL);
  gearman_client_add_server(&client, NULL, WORKER_TEST_PORT);
  ret= gearman_client_do_background(&client, "abandoned_worker", NULL, NULL, 0,
                                    job_handle);
  if (ret != GEARMAN_SUCCESS)
  {
    printf("abandoned_worker_test:%s\n", gearman_client_error(&client));
    return TEST_FAILURE;
  }
  gearman_client_free(&client);

  /* Now take job with one worker. */
  if (gearman_universal_create(&gearman, NULL) == NULL)
    return TEST_FAILURE;

  if (gearman_connection_create(&gearman, &worker1, NULL) == NULL)
    return TEST_FAILURE;

  gearman_connection_set_host(&worker1, NULL, WORKER_TEST_PORT);

  args[0]= "abandoned_worker";
  args_size[0]= strlen("abandoned_worker");
  if (gearman_packet_create_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                 GEARMAN_COMMAND_CAN_DO,
                                 args, args_size, 1) != GEARMAN_SUCCESS)
  {
    return TEST_FAILURE;
  }

  if (gearman_connection_send(&worker1, &packet, true) != GEARMAN_SUCCESS)
    return TEST_FAILURE;

  gearman_packet_free(&packet);

  if (gearman_packet_create_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                 GEARMAN_COMMAND_GRAB_JOB,
                                 NULL, NULL, 0) != GEARMAN_SUCCESS)
  {
    return TEST_FAILURE;
  }

  if (gearman_connection_send(&worker1, &packet, true) != GEARMAN_SUCCESS)
    return TEST_FAILURE;

  gearman_packet_free(&packet);

  gearman_connection_recv(&worker1, &packet, &ret, false);
  if (ret != GEARMAN_SUCCESS || packet.command != GEARMAN_COMMAND_JOB_ASSIGN)
    return TEST_FAILURE;

  if (strcmp(job_handle, packet.arg[0]))
  {
    printf("unexpected job: %s != %s\n", job_handle, (char *)packet.arg[0]);
    return TEST_FAILURE;
  }

  gearman_packet_free(&packet);

  if (gearman_connection_create(&gearman, &worker2, NULL) == NULL)
    return TEST_FAILURE;

  gearman_connection_set_host(&worker2, NULL, WORKER_TEST_PORT);

  args[0]= "abandoned_worker";
  args_size[0]= strlen("abandoned_worker");
  if (gearman_packet_create_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                 GEARMAN_COMMAND_CAN_DO,
                                 args, args_size, 1) != GEARMAN_SUCCESS)
  {
    return TEST_FAILURE;
  }

  if (gearman_connection_send(&worker2, &packet, true) != GEARMAN_SUCCESS)
    return TEST_FAILURE;

  gearman_packet_free(&packet);

  args[0]= job_handle;
  args_size[0]= strlen(job_handle) + 1;
  args[1]= "test";
  args_size[1]= 4;
  if (gearman_packet_create_args(&gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                 GEARMAN_COMMAND_WORK_COMPLETE,
                                 args, args_size, 2) != GEARMAN_SUCCESS)
  {
    return TEST_FAILURE;
  }

  if (gearman_connection_send(&worker2, &packet, true) != GEARMAN_SUCCESS)
    return TEST_FAILURE;

  gearman_packet_free(&packet);

  gearman_universal_set_timeout(&gearman, 1000);
  gearman_connection_recv(&worker2, &packet, &ret, false);
  if (ret != GEARMAN_SUCCESS || packet.command != GEARMAN_COMMAND_ERROR)
    return TEST_FAILURE;

  gearman_packet_free(&packet);
  gearman_universal_free(&gearman);

  return TEST_SUCCESS;
}

static void *_gearman_worker_add_function_worker_fn(gearman_job_st *job __attribute__((unused)),
						    void *context __attribute__((unused)),
						    size_t *result_size __attribute__((unused)),
						    gearman_return_t *ret_ptr __attribute__((unused)))
{
  (void)job;
  (void)context;
  *ret_ptr= GEARMAN_WORK_FAIL;

  return NULL;
}

static test_return_t gearman_worker_add_function_test(void *object)
{
  bool found;
  gearman_return_t rc;
  gearman_worker_st *worker= (gearman_worker_st *)object;
  const char *function_name= "_gearman_worker_add_function_worker_fn";

  rc= gearman_worker_add_function(worker,
				  function_name,
				  0, _gearman_worker_add_function_worker_fn, NULL);
  test_truth(rc == GEARMAN_SUCCESS);

  found= gearman_worker_function_exist(worker, function_name, strlen(function_name));
  test_truth(found);

  rc= gearman_worker_unregister(worker, function_name);
  test_truth(rc == GEARMAN_SUCCESS);

  found= gearman_worker_function_exist(worker, function_name, strlen(function_name));
  test_false(found);

  /* Make sure we have removed it */
  rc= gearman_worker_unregister(worker, function_name);
  test_truth(rc == GEARMAN_NO_REGISTERED_FUNCTION);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_add_function_multi_test(void *object)
{
  uint32_t x;
  gearman_return_t rc;
  gearman_worker_st *worker= (gearman_worker_st *)object;
  const char *function_name_ext= "_gearman_worker_add_function_worker_fn";

  for (x= 0; x < 100; x++)
  {
    char buffer[1024];
    snprintf(buffer, 1024, "%u%s", x, function_name_ext);
    rc= gearman_worker_add_function(worker,
                                    buffer,
                                    0, _gearman_worker_add_function_worker_fn, NULL);

    test_truth(rc == GEARMAN_SUCCESS);
  }

  for (x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, 1024, "%u%s", x, function_name_ext);
    rc= gearman_worker_unregister(worker, buffer);
    test_truth(rc == GEARMAN_SUCCESS);
  }

  for (x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, 1024, "%u%s", x, function_name_ext);
    rc= gearman_worker_unregister(worker, buffer);
    test_truth(rc == GEARMAN_NO_REGISTERED_FUNCTION);
  }

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_unregister_all_test(void *object)
{
  uint32_t x;
  gearman_return_t rc;
  gearman_worker_st *worker= (gearman_worker_st *)object;
  const char *function_name_ext= "_gearman_worker_add_function_worker_fn";

  for (x= 0; x < 100; x++)
  {
    char buffer[1024];
    snprintf(buffer, 1024, "%u%s", x, function_name_ext);
    rc= gearman_worker_add_function(worker,
                                    buffer,
                                    0, _gearman_worker_add_function_worker_fn, NULL);

    test_truth(rc == GEARMAN_SUCCESS);
  }

  rc= gearman_worker_unregister_all(worker);

  for (x= 0; x < 100; x++)
  {
    char buffer[1024];

    snprintf(buffer, 1024, "%u%s", x, function_name_ext);
    rc= gearman_worker_unregister(worker, buffer);
    test_truth(rc == GEARMAN_NO_REGISTERED_FUNCTION);
  }

  rc= gearman_worker_unregister_all(worker);
  test_truth(rc == GEARMAN_NO_REGISTERED_FUNCTIONS);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_work_with_test(void *object)
{
  gearman_return_t rc;
  gearman_worker_st *worker= (gearman_worker_st *)object;
  const char *function_name= "_gearman_worker_add_function_worker_fn";

  rc= gearman_worker_add_function(worker,
				  function_name,
				  0, _gearman_worker_add_function_worker_fn, NULL);
  test_truth(rc == GEARMAN_SUCCESS);

  gearman_worker_set_timeout(worker, 2);

  rc= gearman_worker_work(worker);
  test_truth(rc == GEARMAN_TIMEOUT);

  /* Make sure we have remove worker function */
  rc= gearman_worker_unregister(worker, function_name);
  test_truth(rc == GEARMAN_SUCCESS);

  return TEST_SUCCESS;
}

static test_return_t gearman_worker_context_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  int value= 5;
  int *ptr;

  ptr= gearman_worker_context(worker);
  test_truth(ptr == NULL);

  gearman_worker_set_context(worker, &value);

  ptr= (int *)gearman_worker_context(worker);

  test_truth(ptr == &value);
  test_truth(*ptr == value);

  return TEST_SUCCESS;
}

/*********************** World functions **************************************/

void *create(void *object __attribute__((unused)))
{
  worker_test_st *test= (worker_test_st *)object;
  return (void *)&(test->worker);
}

void *world_create(test_return_t *error)
{
  worker_test_st *test;
  pid_t gearmand_pid;

  gearmand_pid= test_gearmand_start(WORKER_TEST_PORT, NULL, NULL, 0);

  test= malloc(sizeof(worker_test_st));

  if (! test)
  {
    *error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  memset(test, 0, sizeof(worker_test_st));

  test->gearmand_pid= gearmand_pid;

  if (gearman_worker_create(&(test->worker)) == NULL)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  if (gearman_worker_add_server(&(test->worker), NULL, WORKER_TEST_PORT) != GEARMAN_SUCCESS)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  *error= TEST_SUCCESS;

  return (void *)test;
}

test_return_t world_destroy(void *object)
{
  worker_test_st *test= (worker_test_st *)object;
  test_gearmand_stop(test->gearmand_pid);
  gearman_worker_free(&(test->worker));
  free(test);

  return TEST_SUCCESS;
}

test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"clone", 0, clone_test },
  {"echo", 0, echo_test },
  {"echo_multi", 0, echo_multi_test },
  {"options", 0, option_test },
  {"gearman_worker_add_function", 0, gearman_worker_add_function_test },
  {"gearman_worker_add_function_multi", 0, gearman_worker_add_function_multi_test },
  {"gearman_worker_unregister_all", 0, gearman_worker_unregister_all_test },
  {"gearman_worker_work with timout", 0, gearman_worker_work_with_test },
  {"gearman_worker_context", 0, gearman_worker_context_test },
  {"echo_max", 0, echo_max_test },
  {"abandoned_worker", 0, abandoned_worker_test },
  {0, 0, 0}
};

collection_st collection[] ={
  {"worker", 0, 0, tests},
  {0, 0, 0, 0}
};


typedef test_return_t (*libgearman_test_callback_fn)(gearman_worker_st *);
static test_return_t _runner_default(libgearman_test_callback_fn func, worker_test_st *container)
{
  gearman_worker_st *worker= &container->worker;

  if (func)
  {
    (void)gearman_worker_unregister_all(worker);
    return func(worker);
  }
  else
  {
    return TEST_SUCCESS;
  }
}


static world_runner_st runner= {
  (test_callback_runner_fn)_runner_default,
  (test_callback_runner_fn)_runner_default,
  (test_callback_runner_fn)_runner_default
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
  world->runner= &runner;
}
