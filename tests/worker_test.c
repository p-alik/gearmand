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

void *create(void *not_used);
void destroy(void *object);
test_return pre(void *object);
test_return post(void *object);
test_return flush(void);

test_return init_test(void *not_used __attribute__((unused)))
{
  gearman_worker_st worker;

  if (gearman_worker_create(&worker) == NULL)
    return TEST_FAILURE;

  gearman_worker_free(&worker);

  return TEST_SUCCESS;
}

test_return allocation_test(void *not_used __attribute__((unused)))
{
  gearman_worker_st *worker;

  worker= gearman_worker_create(NULL);
  if (worker == NULL)
    return TEST_FAILURE;

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

test_return clone_test(void *object)
{
  gearman_worker_st *from= (gearman_worker_st *)object;
  gearman_worker_st *worker;

  if (gearman_worker_clone(NULL, NULL) != NULL)
    return TEST_FAILURE;

  worker= gearman_worker_clone(NULL, from);
  if (worker == NULL)
    return TEST_FAILURE;

  gearman_worker_free(worker);

  return TEST_SUCCESS;
}

test_return echo_test(void *object __attribute__((unused)))
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  gearman_return_t rc;
  size_t value_length;
  char *value= "This is my echo test";
  
  value_length= strlen(value);
  
  rc= gearman_worker_echo(worker, (uint8_t *)value, value_length);
  if (rc != GEARMAN_SUCCESS)
    return TEST_FAILURE;
  
  return TEST_SUCCESS;
}

#ifdef NOT_DONE
/* Prototype */
uint8_t* simple_worker(gearman_worker_st *job,
                       uint8_t *value,  
                       ssize_t value_length,  
                       ssize_t *result_length,  
                       gearman_return *error);

uint8_t* simple_worker(gearman_worker_st *job,
                       uint8_t *value,  
                       ssize_t value_length,  
                       ssize_t *result_length,  
                       gearman_return *error)
{
  fprintf(stderr, "%.*s\n", value_length, value);

  (void)job;

  *error= GEARMAN_SUCCESS;
  *result_length= strlen("successful");

  return (uint8_t *)strdup("successful");
}

static test_return simple_work_test(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  gearman_worker_function callback[1];

  callback[0]= simple_worker;
  gearman_server_function_register(worker, "simple", callback);
  gearman_server_work(worker);

  return TEST_SUCCESS;
}
#endif

test_return flush(void)
{
  return TEST_SUCCESS;
}

void *create(void *not_used __attribute__((unused)))
{
  gearman_worker_st *worker;
  gearman_return_t rc;

  worker= gearman_worker_create(NULL);

  assert(worker);

  rc= gearman_worker_add_server(worker, NULL, 0);

  assert(rc == GEARMAN_SUCCESS);

  return (void *)worker;
}

void destroy(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;

  assert(worker);

  gearman_worker_free(worker);
}

test_return pre(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;

  assert(worker);

  return TEST_SUCCESS;
}

test_return post(void *object)
{
  gearman_worker_st *worker= (gearman_worker_st *)object;

  assert(worker);

  return TEST_SUCCESS;
}

/* Clean the server before beginning testing */
test_st tests[] ={
  {"init", 0, init_test },
  {"allocation", 0, allocation_test },
  {"clone_test", 0, clone_test },
  {"echo", 0, echo_test },
#ifdef NOT_DONE
  {"simple_work_test", 0, simple_work_test },
#endif
  {0, 0, 0}
};

collection_st collection[] ={
  {"worker", flush, create, destroy, pre, post, tests},
  {0, 0, 0, 0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= NULL;
  world->destroy= NULL;
}
