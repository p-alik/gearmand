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
test_return init_test(void *object);
test_return allocation_test(void *object);
test_return clone_test(void *object);
test_return echo_test(void *object);
test_return submit_job_test(void *object);
test_return background_failure_test(void *object);
test_return background_test(void *object);
test_return error_test(void *object);

void *create(void *object);
void destroy(void *object);
test_return pre(void *object);
test_return post(void *object);
test_return flush(void);

void *client_test_worker(gearman_job_st *job, void *cb_arg, size_t *result_size,
                         gearman_return_t *ret_ptr);
void *world_create(void);
void world_destroy(void *object);

test_return init_test(void *object __attribute__((unused)))
{
  gearman_client_st client;

  if (gearman_client_create(&client) == NULL)
    return TEST_FAILURE;

  gearman_client_free(&client);

  return TEST_SUCCESS;
}

test_return allocation_test(void *object __attribute__((unused)))
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
  {
    printf("echo_test:%s\n", gearman_client_error(client));
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

test_return background_failure_test(void *object)
{
  gearman_return_t rc;
  gearman_client_st *client= (gearman_client_st *)object;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  bool is_known;
  bool is_running;
  uint32_t numerator;
  uint32_t denominator;
  uint8_t *value= (uint8_t *)"background_failure_test";
  ssize_t value_length= strlen("background_failure_test");

  rc= gearman_client_do_background(client, "does_not_exist", NULL, value,
                                   value_length, job_handle);
  if (rc != GEARMAN_SUCCESS)
    return TEST_FAILURE;

  rc= gearman_client_task_status(client, job_handle, &is_known, &is_running,
                                 &numerator, &denominator);
  if (rc != GEARMAN_SUCCESS || is_known != true || is_running != false ||
      numerator != 0 || denominator != 0)
  {
    printf("background_failure_test:%s\n", gearman_client_error(client));
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}

test_return background_test(void *object)
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
    printf("echo_test:%s\n", gearman_client_error(client));
    return TEST_FAILURE;
  }

  while (1)
  {
    rc= gearman_client_task_status(client, job_handle, &is_known, &is_running,
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

test_return submit_job_test(void *object)
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
    printf("echo_test:%s\n", gearman_client_error(client));
    return TEST_FAILURE;
  }

  if (job_result == NULL)
    return TEST_FAILURE;

  if (value_length != job_length || memcmp(value, job_result, value_length))
    return TEST_FAILURE;

  free(job_result);

  return TEST_SUCCESS;
}

test_return flush(void)
{
  return TEST_SUCCESS;
}

void *create(void *object)
{
  client_test_st *test= (client_test_st *)object;
  return (void *)&(test->client);
}

void destroy(void *object __attribute__((unused)))
{
}

test_return pre(void *object __attribute__((unused)))
{
  return TEST_SUCCESS;
}

test_return post(void *object __attribute__((unused)))
{
  return TEST_SUCCESS;
}


void *client_test_worker(gearman_job_st *job, void *cb_arg, size_t *result_size,
                         gearman_return_t *ret_ptr)
{
  const uint8_t *workload;
  uint8_t *result;
  (void)cb_arg;

  workload= gearman_job_workload(job);
  *result_size= gearman_job_workload_size(job);

  assert((result= malloc(*result_size)) != NULL);
  memcpy(result, workload, *result_size);

  ret_ptr= GEARMAN_SUCCESS;
  return result;
}

void *world_create(void)
{
  client_test_st *test;

  assert((test= malloc(sizeof(client_test_st))) != NULL);
  memset(test, 0, sizeof(client_test_st));
  assert(gearman_client_create(&(test->client)) != NULL);

  assert(gearman_client_add_server(&(test->client), NULL, CLIENT_TEST_PORT) ==
         GEARMAN_SUCCESS);

  test->gearmand_pid= test_gearmand_start(CLIENT_TEST_PORT);
  test->worker_pid= test_worker_start(CLIENT_TEST_PORT, "client_test",
                                      client_test_worker, NULL);

  return (void *)test;
}

void world_destroy(void *object)
{
  client_test_st *test= (client_test_st *)object;
  gearman_client_free(&(test->client));
  test_worker_stop(test->worker_pid);
  test_gearmand_stop(test->gearmand_pid);
  free(test);
}

test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"clone_test", 0, clone_test },
  {"echo", 0, echo_test },
  {"background_failure", 0, background_failure_test },
  {"background", 0, background_test },
  {"submit_job", 0, submit_job_test },
  {0, 0, 0}
};

collection_st collection[] ={
  {"client", flush, create, destroy, pre, post, tests},
  {0, 0, 0, 0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
}
