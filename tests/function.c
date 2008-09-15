/*
  Sample test application.
*/
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

test_return init_test(void *not_used)
{
  gearman_st object;

  (void)gearman_create(&object);
  gearman_free(&object);

  return TEST_SUCCESS;
}

test_return allocation_test(void *not_used)
{
  gearman_st *object;
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
  assert(rc == GEARMAN_SUCCESS);

  return TEST_SUCCESS;
}

test_return submit_job_test(void *object)
{
  gearman_return rc;
  gearman_st *param= (gearman_st *)object;
  gearman_result_st *result;
  gearman_job_st *job;
  char *value= "submit_job_test";
  size_t value_length= strlen("submit_job_test");

  result= gearman_result_create(param, NULL);
  job= gearman_job_create(param, NULL);

  assert(result);
  assert(job);


  gearman_job_set_function(job, "echo");
  gearman_job_set_value(job, value, value_length);

  rc= gearman_job_submit(job);

  assert(rc == GEARMAN_SUCCESS);
  WATCHPOINT;

  rc= gearman_job_result(job, result);

  assert(rc == GEARMAN_SUCCESS);
  assert(result->action == GEARMAN_WORK_COMPLETE);
  assert(gearman_result_length(result) == value_length);
  assert(memcmp(gearman_result_value(result), value, value_length) == 0);

  return TEST_SUCCESS;
}

test_return background_test(void *object)
{
  gearman_return rc;
  gearman_st *param= (gearman_st *)object;
  gearman_result_st *result;
  gearman_job_st *job;
  char *value= "background_test";
  size_t value_length= strlen("background_test");

  result= gearman_result_create(param, NULL);
  job= gearman_job_create(param, NULL);

  assert(result);
  assert(job);

  gearman_job_set_function(job, "echo");
  gearman_job_set_value(job, value, value_length);
  rc= gearman_job_set_behavior(job, GEARMAN_BEHAVIOR_JOB_BACKGROUND, 1);
  assert(GEARMAN_SUCCESS == rc);

  rc= gearman_job_submit(job);

  assert(rc == GEARMAN_SUCCESS);

  do {
    rc= gearman_job_result(job, result);

    assert(rc == GEARMAN_SUCCESS ||  rc == GEARMAN_STILL_RUNNING);

    if (rc == GEARMAN_SUCCESS)
      break;
  } while (1);

  gearman_result_free(result);
  gearman_job_free(job);

  return TEST_SUCCESS;
}

test_return error_test(void *object)
{
  gearman_return rc;
  gearman_st *param= (gearman_st *)object;

  for (rc= GEARMAN_SUCCESS; rc < GEARMAN_MAXIMUM_RETURN; rc++)
  {
    char *string;
    string= gearman_strerror(param, rc);
  }

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
  {"error", 0, error_test },
  {"echo", 0, echo_test },
  {"submit_job", 0, submit_job_test },
  {"submit_job2", 0, submit_job_test },
  {"background", 0, background_test },
  {0, 0, 0}
};

collection_st collection[] ={
  {"norm", flush, create, destroy, pre, post, tests},
  {0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= NULL;
  world->destroy= NULL;
}
