/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
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
#include <stdbool.h>

#include <libgearman/gearman.h>

#include "test.h"

/* Prototypes */
test_return init_test(void *not_used);
test_return allocation_test(void *not_used);
test_return clone_test(void *object);
test_return echo_test(void *object);
test_return submit_job_test(void *object);
test_return background_failure_test(void *object);
test_return background_test(void *object);
test_return error_test(void *object);

void *create(void *not_used);
void destroy(void *object);
test_return pre(void *object);
test_return post(void *object);
test_return flush(void);

test_return init_test(void *not_used __attribute__((unused)))
{
  gearman_client_st client;

  if (gearman_client_create(&client) == NULL)
    return TEST_FAILURE;

  gearman_client_free(&client);

  return TEST_SUCCESS;
}

test_return allocation_test(void *not_used __attribute__((unused)))
{
  gearman_client_st *client;

  client= gearman_client_create(NULL);
  if (client == NULL)
    return TEST_FAILURE;

  gearman_client_free(client);

  return TEST_SUCCESS;
}

test_return clone_test(void *object)
{
  gearman_client_st *from= (gearman_client_st *)object;
  gearman_client_st *client;

  if (gearman_client_clone(NULL, NULL) != NULL)
    return TEST_FAILURE;

  client= gearman_client_clone(NULL, from);
  if (client == NULL)
    return TEST_FAILURE;

  gearman_client_free(client);

  return TEST_SUCCESS;
}

test_return echo_test(void *object __attribute__((unused)))
{
  gearman_client_st *client= (gearman_client_st *)object;
  gearman_return_t rc;
  size_t value_length;
  char *value= "This is my echo test";

  value_length= strlen(value);

  rc= gearman_client_echo(client, (uint8_t *)value, value_length);
  if (rc != GEARMAN_SUCCESS)
    return TEST_FAILURE;

  return TEST_SUCCESS;
}

test_return background_failure_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  gearman_job_handle_t job_handle;
  bool is_known;
  bool is_running;
  uint32_t numerator;
  uint32_t denominator;
  uint8_t *value= (uint8_t *)"background_failure_test";
  ssize_t value_length= strlen("background_failure_test");

  rc= gearman_client_do_background(client, "does_not_exist", value,
                                   value_length, job_handle);
  if (rc != GEARMAN_SUCCESS)
    return TEST_FAILURE;

  rc= gearman_client_task_status(client, job_handle, &is_known, &is_running,
                                 &numerator, &denominator);
  if (rc != GEARMAN_SUCCESS || is_known != true || is_running != false ||
      numerator != 0 || denominator != 0)
  {
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

test_return background_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  gearman_job_handle_t job_handle;
  bool is_known;
  bool is_running;
  uint32_t numerator;
  uint32_t denominator;
  uint8_t *value= (uint8_t *)"background_test";
  size_t value_length= strlen("background_test");

  rc= gearman_client_do_background(client, "frog",
                                   value, value_length, job_handle);
  //WATCHPOINT_ERROR(rc);
  assert(rc == GEARMAN_SUCCESS);
  //sleep(1); /* Yes, this could fail on an overloaded system to give the
  //server enough time to assign */

  rc= gearman_client_task_status(client, job_handle, &is_known, &is_running, &numerator, &denominator);
  assert(rc == GEARMAN_SUCCESS);
  assert(is_known == true);

  while (1)
  {
    rc= gearman_client_task_status(client, job_handle, &is_known, &is_running, &numerator, &denominator);
    assert(rc == GEARMAN_SUCCESS);
    if (is_running == true)
    {
      //WATCHPOINT_NUMBER(numerator);
      //WATCHPOINT_NUMBER(denominator);
      continue;
    }
    else
      break;
  }

  return TEST_SUCCESS;
}

test_return submit_job_test(void *object)
{
(void)object;
#if 0
  gearman_return rc;
  gearman_client_st *client= (gearman_client_st *)object;
  uint8_t *job_result;
  ssize_t job_length;
  uint8_t *value= (uint8_t *)"submit_job_test";
  size_t value_length= strlen("submit_job_test");

  job_result= gearman_client_do(client, "frog",
                                value, value_length, &job_length, &rc);
  WATCHPOINT_ERROR(rc);
  assert(rc == GEARMAN_SUCCESS);
  assert(job_result);
  WATCHPOINT_STRING_LENGTH((char *)job_result, job_length);

  if (job_result)
    free(job_result);

#endif
  return TEST_SUCCESS;
}

#ifdef NOT_DONE
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
#endif

test_return flush(void)
{
  return TEST_SUCCESS;
}

void *create(void *not_used __attribute__((unused)))
{
  gearman_client_st *client;
  gearman_return_t rc;

  client= gearman_client_create(NULL);

  assert(client);

  rc= gearman_client_add_server(client, NULL, 0);

  assert(rc == GEARMAN_SUCCESS);

  return (void *)client;
}

void destroy(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  assert(client);

  gearman_client_free(client);
}

test_return pre(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  assert(client);

  return TEST_SUCCESS;
}

test_return post(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  assert(client);

  return TEST_SUCCESS;
}

/* Clean the server before beginning testing */
test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"clone_test", 0, clone_test },
  {"echo", 0, echo_test },
  {"background_failure", 0, background_failure_test },
#ifdef NOT_DONE
  {"submit_job", 0, submit_job_test },
  {"background", 0, background_test },
  {"error", 0, error_test },
  {"submit_job2", 0, submit_job_test },
#endif
  {0, 0, 0}
};

collection_st collection[] ={
  {"client", flush, create, destroy, pre, post, tests},
  {0, 0, 0, 0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= NULL;
  world->destroy= NULL;
}
