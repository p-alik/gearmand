/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define GEARMAN_INTERNAL 1
#include <libgearman/gearman.h>

#include "test.h"

/* Prototypes */
test_return init_test(void *not_used);
test_return allocation_test(void *not_used);
test_return clone_test(void *object);
test_return echo_test(void *object);
test_return worker_id_test(void *object);
test_return worker_function_null_test(void *object);
test_return worker_function_timeout_test(void *object);
test_return worker_function_test(void *object);
void *create(void *not_used);
void destroy(void *object);
test_return pre(void *object);
test_return post(void *object);
test_return flush(void);

test_return init_test(void *not_used)
{
  gearman_st object;
  not_used= NULL; /* suppress compiler warnings */

  (void)gearman_create(&object);
  gearman_free(&object);

  return TEST_SUCCESS;
}

test_return allocation_test(void *not_used)
{
  gearman_st *object;
  not_used= NULL; /* suppress compiler warnings */
  object= gearman_create(NULL);
  assert(object);
  gearman_free(object);

  return TEST_SUCCESS;
}

test_return clone_test(void *object)
{
  gearman_st *param= (gearman_st *)object;

  /* All null? */
  {
    gearman_st *clone;
    clone= gearman_clone(NULL, NULL);
    assert(clone);
    gearman_free(clone);
  }

  /* Can we init from null? */
  {
    gearman_st *clone;
    clone= gearman_clone(NULL, param);
    assert(clone);
    gearman_free(clone);
  }

  /* Can we init from struct? */
  {
    gearman_st declared_clone;
    gearman_st *clone;
    clone= gearman_clone(&declared_clone, NULL);
    assert(clone);
    gearman_free(clone);
  }

  /* Can we init from struct? */
  {
    gearman_st declared_clone;
    gearman_st *clone;
    clone= gearman_clone(&declared_clone, param);
    assert(clone);
    gearman_free(clone);
  }

  return TEST_SUCCESS;
}

test_return echo_test(void *object)
{
  gearman_return rc;
  gearman_st *param= (gearman_st *)object;
  size_t value_length;
  char *value= "This is my echo test";

  value_length= strlen(value);

  rc= gearman_echo(param, value, value_length, NULL);
  WATCHPOINT_ERROR(rc);
  assert(rc == GEARMAN_SUCCESS);

  return TEST_SUCCESS;
}

test_return worker_id_test(void *object)
{
  gearman_return rc;
  gearman_st *param= (gearman_st *)object;
  gearman_worker_st *worker;
  char *function_name= "id_function";

  worker= gearman_worker_create(param, NULL);
  assert(worker);

  rc= gearman_worker_do(worker, function_name);
  assert(rc == GEARMAN_SUCCESS);

  rc= gearman_worker_set_id(worker, "kungfu");
  assert(rc == GEARMAN_SUCCESS);

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

test_return worker_function_null_test(void *object)
{
  gearman_return rc;
  gearman_st *param= (gearman_st *)object;
  gearman_worker_st *worker;
  gearman_result_st *result;
  char *function_name= "bad_function";

  worker= gearman_worker_create(param, NULL);
  assert(worker);

  result= gearman_result_create(param, NULL);
  assert(result);

  WATCHPOINT;
  rc= gearman_worker_do(worker, function_name);
  assert(rc == GEARMAN_SUCCESS);

  WATCHPOINT;
  rc= gearman_worker_take(worker, result);

  assert(rc == GEARMAN_NOT_FOUND);
  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

test_return worker_function_timeout_test(void *object)
{
  gearman_return rc;
  gearman_st *param= (gearman_st *)object;
  gearman_worker_st *worker;
  gearman_result_st *result;
  char *function_name= "bad_function";

  worker= gearman_worker_create(param, NULL);
  assert(worker);

  result= gearman_result_create(param, NULL);
  assert(result);

  gearman_worker_set_timeout(worker, (time_t)23);
  rc= gearman_worker_do(worker, function_name);
  assert(rc == GEARMAN_SUCCESS);

  rc= gearman_worker_take(worker, result);

  assert(rc == GEARMAN_NOT_FOUND);
  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

test_return worker_function_test(void *object)
{
  uint8_t loop_count;
  gearman_return rc;
  gearman_st *param= (gearman_st *)object;
  gearman_worker_st *worker;
  gearman_job_st *job;
  gearman_result_st *result;
  char *function_name= "echo";
  char *value= "worker_function_test";
  size_t value_length= strlen("worker_function_test");
  char *return_value= "this is working";
  size_t return_value_length= strlen("this is working");

  worker= gearman_worker_create(param, NULL);
  job= gearman_job_create(param, NULL);
  result= gearman_result_create(param, NULL);
  assert(worker);
  assert(job);
  assert(result);

  rc= gearman_worker_do(worker, function_name);
  assert(rc == GEARMAN_SUCCESS);
  WATCHPOINT_ERROR(rc);

  /* submit the job */
  {
    gearman_job_set_function(job, "echo");
    gearman_job_set_value(job, value, value_length);
    assert(GEARMAN_SUCCESS == rc);

    rc= gearman_job_submit(job);
    assert(rc == GEARMAN_SUCCESS);
    WATCHPOINT_ERROR(rc);
  }

  /* Loop until we take a job */
  do {
    rc= gearman_worker_take(worker, result);
  } while (rc != GEARMAN_SUCCESS);
  assert(gearman_result_length(result) == value_length);
  assert(memcmp(gearman_result_value(result), value, value_length) == 0);
  assert(gearman_result_handle(result));

  /* Before we handle check on status with job */
  loop_count= 3;
  while (loop_count--)
  {
    rc= gearman_job_result(job, result);
  }

  rc= gearman_result_set_value(result, return_value, return_value_length);
  assert(rc == GEARMAN_SUCCESS);

  assert(gearman_result_handle(result));
  rc= gearman_worker_answer(worker, result);
  assert(rc == GEARMAN_SUCCESS);

  do {
    rc= gearman_job_result(job, result);

    assert(rc == GEARMAN_SUCCESS ||  rc == GEARMAN_STILL_RUNNING);

    if (rc == GEARMAN_SUCCESS)
      break;
  } while (1);

  assert(rc == GEARMAN_SUCCESS);
  assert(result->action == GEARMAN_WORK_COMPLETE);
  assert(gearman_result_length(result) == return_value_length);
  assert(memcmp(gearman_result_value(result), return_value, return_value_length) == 0);

  gearman_job_free(job);
  gearman_result_free(result);
  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

test_return flush(void)
{
  return TEST_SUCCESS;
}

void *create(void *not_used)
{
  gearman_st *ptr;
  gearman_return rc;
  not_used= NULL; /* suppress compiler warnings */

  ptr= gearman_create(NULL);

  assert(ptr);

  rc= gearman_server_add(ptr, "localhost", 0);

  assert(rc == GEARMAN_SUCCESS);


  return (void *)ptr;
}

void destroy(void *object)
{
  gearman_st *ptr= (gearman_st *)object;

  assert(ptr);

  gearman_free(object);
}

test_return pre(void *object)
{
  gearman_st *ptr= (gearman_st *)object;

  assert(ptr);

  return TEST_SUCCESS;
}

test_return post(void *object)
{
  gearman_st *ptr= (gearman_st *)object;

  assert(ptr);

  return TEST_SUCCESS;
}

/* Clean the server before beginning testing */
test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"clone_test", 0, clone_test },
  {"echo", 0, echo_test },
  {"worker_id", 0, worker_id_test },
  {"worker_function_null", 0, worker_function_null_test },
  {"worker_function_timeout", 0, worker_function_timeout_test },
  {"worker_function", 0, worker_function_test },
  {0, 0, 0}
};

collection_st collection[] ={
  {"norm", flush, create, destroy, pre, post, tests},
  {0, 0, 0, 0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= NULL;
  world->destroy= NULL;
}
