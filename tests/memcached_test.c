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
#include <unistd.h>

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
test_return queue_add(void *object);
test_return queue_worker(void *object);

void *create(void *object);
void destroy(void *object);
test_return pre(void *object);
test_return post(void *object);
test_return flush(void);

void *world_create(void);
void world_destroy(void *object);

/* Counter test for worker */
static void *counter_function(gearman_job_st *job __attribute__((unused)), 
                              void *context, 
                              size_t *result_size,
                              gearman_return_t *ret_ptr __attribute__((unused)))
{
  uint32_t *counter= (uint32_t *)context;

  *result_size= 0;

  *counter= *counter + 1;

  return NULL;
}

test_return queue_add(void *object __attribute__((unused)))
{
  gearman_return_t rc;
  gearman_client_st client;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  uint8_t *value= (uint8_t *)"background_test";
  size_t value_length= strlen("background_test");

  memset(&client, 0, sizeof(gearman_client_st));
  assert(gearman_client_create(&client) != NULL);

  assert((gearman_client_add_server(&client, "localhost", WORKER_TEST_PORT)) == GEARMAN_SUCCESS);

  rc= gearman_client_do_background(&client, "queue_test", NULL, value,
                                   value_length, job_handle);
  assert(rc == GEARMAN_SUCCESS);

  gearman_client_free(&client);

  return TEST_SUCCESS;
}

test_return queue_worker(void *object __attribute__((unused)))
{
  gearman_worker_st *worker= (gearman_worker_st *)object;
  uint32_t counter= 0;

  assert((gearman_worker_add_function(worker, "queue_test", 5, counter_function, &counter)) == GEARMAN_SUCCESS);

  assert((gearman_worker_work(worker)) == GEARMAN_SUCCESS);

  assert(counter > 0);

  return TEST_SUCCESS;
}


test_return flush(void)
{
  return TEST_SUCCESS;
}

void *create(void *object __attribute__((unused)))
{
  worker_test_st *test= (worker_test_st *)object;
  return (void *)&(test->worker);
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

void *world_create(void)
{
  worker_test_st *test;
  char *argv[1];

  argv[0]= "servers=localhost";

  assert((test= malloc(sizeof(worker_test_st))) != NULL);
  memset(test, 0, sizeof(worker_test_st));
  assert(gearman_worker_create(&(test->worker)) != NULL);

  assert(gearman_worker_add_server(&(test->worker), NULL, WORKER_TEST_PORT) ==
         GEARMAN_SUCCESS);

  test->gearmand_pid= test_gearmand_start(WORKER_TEST_PORT, "libmemcached", argv, 1);

  return (void *)test;
}

void world_destroy(void *object)
{
  worker_test_st *test= (worker_test_st *)object;
  gearman_worker_free(&(test->worker));
  test_gearmand_stop(test->gearmand_pid);
  free(test);
}

test_st tests[] ={
  {"add", 0, queue_add },
  {"worker", 0, queue_worker },
  {0, 0, 0}
};

collection_st collection[] ={
#ifdef HAVE_LIBMEMCACHED
  {"memcached queue", flush, create, destroy, pre, post, tests},
#endif
  {0, 0, 0, 0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
}
