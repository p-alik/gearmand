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

#include <libgearman/gearman.h>

#include "test.h"
#include "test_gearmand.h"
#include "test_worker.h"

#define CLIENT_TEST_PORT 32123

typedef struct
{
  gearman_client_st client;
  pid_t gearmand_pid;
  pid_t worker_pid;
} client_test_st;

/* Prototypes */
test_return_t init_test(void *object);
test_return_t allocation_test(void *object);
test_return_t clone_test(void *object);
test_return_t echo_test(void *object);
test_return_t submit_job_test(void *object);
test_return_t submit_null_job_test(void *object);
test_return_t submit_fail_job_test(void *object);
test_return_t background_test(void *object);
test_return_t background_failure_test(void *object);
test_return_t add_servers_test(void *object);

void *client_test_worker(gearman_job_st *job, void *context,
                         size_t *result_size, gearman_return_t *ret_ptr);
void *world_create(test_return_t *error);
test_return_t world_destroy(void *object);

test_return_t init_test(void *object __attribute__((unused)))
{
  gearman_client_st client;

  if (gearman_client_create(&client) == NULL)
    return TEST_FAILURE;

  gearman_client_free(&client);

  return TEST_SUCCESS;
}

test_return_t allocation_test(void *object __attribute__((unused)))
{
  gearman_client_st *client;

  client= gearman_client_create(NULL);
  if (client == NULL)
    return TEST_FAILURE;

  gearman_client_free(client);

  return TEST_SUCCESS;
}

test_return_t clone_test(void *object)
{
  gearman_client_st *from= (gearman_client_st *)object;
  gearman_client_st *client;

  client= gearman_client_clone(NULL, NULL);

  test_truth(client);
  test_truth(client->options.allocated);

  gearman_client_free(client);

  client= gearman_client_clone(NULL, from);
  if (client == NULL)
    return TEST_FAILURE;

  gearman_client_free(client);

  return TEST_SUCCESS;
}

static test_return_t option_test(void *object __attribute__((unused)))
{
  gearman_client_st *gear;
  gearman_client_options_t default_options;

  gear= gearman_client_create(NULL);
  test_truth(gear);
  { // Initial Allocated, no changes
    test_truth(gear->options.allocated);
    test_false(gear->options.non_blocking);
    test_false(gear->options.task_in_use);
    test_false(gear->options.unbuffered_result);
    test_false(gear->options.no_new);
    test_false(gear->options.free_tasks);
  }

  /* Set up for default options */
  default_options= gearman_client_options(gear);

  /*
    We take the basic options, and push
    them back in. See if we change anything.
  */
  gearman_client_set_options(gear, default_options);
  { // Initial Allocated, no changes
    test_truth(gear->options.allocated);
    test_false(gear->options.non_blocking);
    test_false(gear->options.task_in_use);
    test_false(gear->options.unbuffered_result);
    test_false(gear->options.no_new);
    test_false(gear->options.free_tasks);
  }

  /*
    We will trying to modify non-mutable options (which should not be allowed)
  */
  {
    gearman_client_remove_options(gear, GEARMAN_CLIENT_ALLOCATED);
    { // Initial Allocated, no changes
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_false(gear->options.task_in_use);
      test_false(gear->options.unbuffered_result);
      test_false(gear->options.no_new);
      test_false(gear->options.free_tasks);
    }
    gearman_client_remove_options(gear, GEARMAN_CLIENT_NO_NEW);
    { // Initial Allocated, no changes
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_false(gear->options.task_in_use);
      test_false(gear->options.unbuffered_result);
      test_false(gear->options.no_new);
      test_false(gear->options.free_tasks);
    }
  }

  /*
    We will test modifying GEARMAN_CLIENT_NON_BLOCKING in several manners.
  */
  {
    gearman_client_remove_options(gear, GEARMAN_CLIENT_NON_BLOCKING);
    { // GEARMAN_CLIENT_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_false(gear->options.task_in_use);
      test_false(gear->options.unbuffered_result);
      test_false(gear->options.no_new);
      test_false(gear->options.free_tasks);
    }
    gearman_client_add_options(gear, GEARMAN_CLIENT_NON_BLOCKING);
    { // GEARMAN_CLIENT_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_false(gear->options.task_in_use);
      test_false(gear->options.unbuffered_result);
      test_false(gear->options.no_new);
      test_false(gear->options.free_tasks);
    }
    gearman_client_set_options(gear, GEARMAN_CLIENT_NON_BLOCKING);
    { // GEARMAN_CLIENT_NON_BLOCKING set to default, by default.
      test_truth(gear->options.allocated);
      test_truth(gear->options.non_blocking);
      test_false(gear->options.task_in_use);
      test_false(gear->options.unbuffered_result);
      test_false(gear->options.no_new);
      test_false(gear->options.free_tasks);
    }
    gearman_client_set_options(gear, GEARMAN_CLIENT_UNBUFFERED_RESULT);
    { // Everything is now set to false except GEARMAN_CLIENT_UNBUFFERED_RESULT, and non-mutable options
      test_truth(gear->options.allocated);
      test_false(gear->options.non_blocking);
      test_false(gear->options.task_in_use);
      test_truth(gear->options.unbuffered_result);
      test_false(gear->options.no_new);
      test_false(gear->options.free_tasks);
    }
    /*
      Reset options to default. Then add an option, and then add more options. Make sure
      the options are all additive.
    */
    {
      gearman_client_set_options(gear, default_options);
      { // See if we return to defaults
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_false(gear->options.task_in_use);
        test_false(gear->options.unbuffered_result);
        test_false(gear->options.no_new);
        test_false(gear->options.free_tasks);
      }
      gearman_client_add_options(gear, GEARMAN_CLIENT_FREE_TASKS);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_false(gear->options.task_in_use);
        test_false(gear->options.unbuffered_result);
        test_false(gear->options.no_new);
        test_truth(gear->options.free_tasks);
      }
      gearman_client_add_options(gear, GEARMAN_CLIENT_NON_BLOCKING|GEARMAN_CLIENT_UNBUFFERED_RESULT);
      { // GEARMAN_CLIENT_NON_BLOCKING set to default, by default.
        test_truth(gear->options.allocated);
        test_truth(gear->options.non_blocking);
        test_false(gear->options.task_in_use);
        test_truth(gear->options.unbuffered_result);
        test_false(gear->options.no_new);
        test_truth(gear->options.free_tasks);
      }
    }
    /*
      Add an option, and then replace with that option plus a new option.
    */
    {
      gearman_client_set_options(gear, default_options);
      { // See if we return to defaults
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_false(gear->options.task_in_use);
        test_false(gear->options.unbuffered_result);
        test_false(gear->options.no_new);
        test_false(gear->options.free_tasks);
      }
      gearman_client_add_options(gear, GEARMAN_CLIENT_FREE_TASKS);
      { // All defaults, except timeout_return
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_false(gear->options.task_in_use);
        test_false(gear->options.unbuffered_result);
        test_false(gear->options.no_new);
        test_truth(gear->options.free_tasks);
      }
      gearman_client_add_options(gear, GEARMAN_CLIENT_FREE_TASKS|GEARMAN_CLIENT_UNBUFFERED_RESULT);
      { // GEARMAN_CLIENT_NON_BLOCKING set to default, by default.
        test_truth(gear->options.allocated);
        test_false(gear->options.non_blocking);
        test_false(gear->options.task_in_use);
        test_truth(gear->options.unbuffered_result);
        test_false(gear->options.no_new);
        test_truth(gear->options.free_tasks);
      }
    }
  }

  gearman_client_free(gear);

  return TEST_SUCCESS;
}

test_return_t echo_test(void *object __attribute__((unused)))
{
  gearman_client_st *client= (gearman_client_st *)object;
  gearman_return_t rc;
  size_t value_length;
  const char *value= "This is my echo test";

  value_length= strlen(value);

  rc= gearman_client_echo(client, (uint8_t *)value, value_length);
  if (rc != GEARMAN_SUCCESS)
  {
    printf("echo_test:%s\n", gearman_client_error(client));
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

test_return_t submit_job_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  uint8_t *job_result;
  size_t job_length;
  uint8_t *value= (uint8_t *)"submit_job_test";
  size_t value_length= strlen("submit_job_test");

  job_result= gearman_client_do(client, "client_test", NULL, value,
                                value_length, &job_length, &rc);
  if (rc != GEARMAN_SUCCESS)
  {
    printf("submit_job_test:%s\n", gearman_client_error(client));
    return TEST_FAILURE;
  }

  if (job_result == NULL)
    return TEST_FAILURE;

  if (value_length != job_length || memcmp(value, job_result, value_length))
    return TEST_FAILURE;

  free(job_result);

  return TEST_SUCCESS;
}

test_return_t submit_null_job_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  uint8_t *job_result;
  size_t job_length;

  job_result= gearman_client_do(client, "client_test", NULL, NULL, 0,
                                &job_length, &rc);
  if (rc != GEARMAN_SUCCESS)
  {
    printf("submit_null_job_test:%s\n", gearman_client_error(client));
    return TEST_FAILURE;
  }

  if (job_result != NULL || job_length != 0)
    return TEST_FAILURE;

  return TEST_SUCCESS;
}

test_return_t submit_fail_job_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  uint8_t *job_result;
  size_t job_length;

  job_result= gearman_client_do(client, "client_test", NULL, "fail", 4,
                                &job_length, &rc);
  if (rc != GEARMAN_WORK_FAIL)
  {
    printf("submit_fail_job_test:%s\n", gearman_client_error(client));
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

test_return_t background_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  bool is_known;
  bool is_running;
  uint32_t numerator;
  uint32_t denominator;
  uint8_t *value= (uint8_t *)"background_test";
  size_t value_length= strlen("background_test");

  rc= gearman_client_do_background(client, "client_test", NULL, value,
                                   value_length, job_handle);
  if (rc != GEARMAN_SUCCESS)
  {
    printf("background_test:%s\n", gearman_client_error(client));
    return TEST_FAILURE;
  }

  while (1)
  {
    rc= gearman_client_job_status(client, job_handle, &is_known, &is_running,
                                  &numerator, &denominator);
    if (rc != GEARMAN_SUCCESS)
    {
      printf("background_test:%s\n", gearman_client_error(client));
      return TEST_FAILURE;
    }

    if (is_known == false)
      break;
  }

  return TEST_SUCCESS;
}

test_return_t background_failure_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  bool is_known;
  bool is_running;
  uint32_t numerator;
  uint32_t denominator;
  uint8_t *value= (uint8_t *)"background_failure_test";
  size_t value_length= strlen("background_failure_test");

  rc= gearman_client_do_background(client, "does_not_exist", NULL, value,
                                   value_length, job_handle);
  if (rc != GEARMAN_SUCCESS)
    return TEST_FAILURE;

  rc= gearman_client_job_status(client, job_handle, &is_known, &is_running,
                                &numerator, &denominator);
  if (rc != GEARMAN_SUCCESS || is_known != true || is_running != false ||
      numerator != 0 || denominator != 0)
  {
    printf("background_failure_test:%s\n", gearman_client_error(client));
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

test_return_t add_servers_test(void *object __attribute__((unused)))
{
  gearman_client_st client;

  if (gearman_client_create(&client) == NULL)
    return TEST_FAILURE;

  if (gearman_client_add_servers(&client, "127.0.0.1:4730,localhost")
      != GEARMAN_SUCCESS)
  {
    return TEST_FAILURE;
  }

  if (gearman_client_add_servers(&client, "old_jobserver:7003,broken:12345")
      != GEARMAN_SUCCESS)
  {
    return TEST_FAILURE;
  }

  gearman_client_free(&client);

  return TEST_SUCCESS;
}

static test_return_t submit_log_failure(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  uint8_t *job_result;
  size_t job_length;
  uint8_t *value= (uint8_t *)"submit_log_failure";
  size_t value_length= strlen("submit_log_failure");

  job_result= gearman_client_do(client, "client_test", NULL, value,
                                value_length, &job_length, &rc);
  if (rc != GEARMAN_SUCCESS)
    return TEST_SUCCESS;

  return TEST_FAILURE;
}

static void log_counter(const char *line, gearman_verbose_t verbose,
                        void *context)
{
  uint32_t *counter= (uint32_t *)context;

  (void)verbose;
  (void)line;

  *counter= *counter + 1;
}

static uint32_t global_counter;
static test_return_t pre_logging(void *object)
{
  client_test_st *all= (client_test_st *)object;
  gearman_log_fn *func= log_counter;
  global_counter= 0;

  gearman_client_set_log_fn(&all->client, func, &global_counter, GEARMAN_VERBOSE_MAX);
  gearman_client_remove_servers(&all->client);

  return TEST_SUCCESS;
}

static test_return_t post_logging(void *object __attribute__((unused)))
{
  test_truth(global_counter);

  return TEST_SUCCESS;
}


void *client_test_worker(gearman_job_st *job, void *context,
                         size_t *result_size, gearman_return_t *ret_ptr)
{
  const uint8_t *workload;
  uint8_t *result;
  (void)context;

  workload= gearman_job_workload(job);
  *result_size= gearman_job_workload_size(job);

  if (workload == NULL || *result_size == 0)
  {
    assert(workload == NULL && *result_size == 0);
    result= NULL;
  }
  else if (*result_size == 4 && !memcmp(workload, "fail", 4))
  {
    *ret_ptr= GEARMAN_WORK_FAIL;
    return NULL;
  }
  else
  {
    assert((result= malloc(*result_size)) != NULL);
    memcpy(result, workload, *result_size);
  }

  *ret_ptr= GEARMAN_SUCCESS;
  return result;
}

void *world_create(test_return_t *error)
{
  client_test_st *test;
  /**
   *  @TODO We cast this to char ** below, which is evil. We need to do the
   *  right thing
   */
  const char *argv[1]= { "client_gearmand" };

  test= malloc(sizeof(client_test_st));
  if (! test)
  {
    *error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  memset(test, 0, sizeof(client_test_st));

  if (gearman_client_create(&(test->client)) == NULL)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  if (gearman_client_add_server(&(test->client), NULL, CLIENT_TEST_PORT) != GEARMAN_SUCCESS)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  test->gearmand_pid= test_gearmand_start(CLIENT_TEST_PORT, NULL,
                                          (char **)argv, 1);
  test->worker_pid= test_worker_start(CLIENT_TEST_PORT, "client_test",
                                      client_test_worker, NULL);

  *error= TEST_SUCCESS;

  return (void *)test;
}


test_return_t world_destroy(void *object)
{
  client_test_st *test= (client_test_st *)object;
  gearman_client_free(&(test->client));
  test_worker_stop(test->worker_pid);
  test_gearmand_stop(test->gearmand_pid);
  free(test);

  return TEST_SUCCESS;
}


test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"clone_test", 0, clone_test },
  {"echo", 0, echo_test },
  {"options", 0, option_test },
  {"submit_job", 0, submit_job_test },
  {"submit_null_job", 0, submit_null_job_test },
  {"submit_fail_job", 0, submit_fail_job_test },
  {"background", 0, background_test },
  {"background_failure", 0, background_failure_test },
  {"add_servers", 0, add_servers_test },
  {0, 0, 0}
};


test_st tests_log[] ={
  {"submit_log_failure", 0, submit_log_failure },
  {0, 0, 0}
};


collection_st collection[] ={
  {"client", 0, 0, tests},
  {"client-logging", pre_logging, post_logging, tests_log},
  {0, 0, 0, 0}
};

typedef test_return_t (*libgearman_test_callback_fn)(gearman_client_st *);
static test_return_t _runner_default(libgearman_test_callback_fn func, client_test_st *container)
{
  if (func)
  {
    return func(&container->client);
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
