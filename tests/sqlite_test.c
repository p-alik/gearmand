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

#include <libgearman/gearman.h>

#include "test.h"
#include "test_gearmand.h"

#define WORKER_TEST_PORT 32123

typedef struct
{
  pid_t gearmand_pid;
  gearman_worker_st worker;
  bool run_worker;
} worker_test_st;

/* Prototypes */
test_return_t echo_test(void *object);
test_return_t queue_add(void *object);
test_return_t queue_worker(void *object);

test_return_t pre(void *object);
test_return_t post(void *object);

void *world_create(test_return_t *error);
test_return_t world_destroy(void *object);

/* Counter test for worker */
static void *counter_function(gearman_job_st *job __attribute__((unused)),
                              void *context, size_t *result_size,
                              gearman_return_t *ret_ptr __attribute__((unused)))
{
  uint32_t *counter= (uint32_t *)context;

  *result_size= 0;

  *counter= *counter + 1;

  return NULL;
}

test_return_t echo_test(void *object)
{
  gearman_client_st client, *client_ptr;
  gearman_return_t rc;
  size_t value_length;
  const char *value= "This is my echo test";

  (void)object;

  value_length= strlen(value);

  client_ptr= gearman_client_create(&client);
  test_truth(client_ptr);

  rc= gearman_client_echo(&client, (uint8_t *)value, value_length);

  test_truth(rc == GEARMAN_SUCCESS);

  gearman_client_free(&client);

  return TEST_SUCCESS;
}

static test_return_t queue_clean(void *object)
{
  worker_test_st *test= (worker_test_st *)object;
  gearman_worker_st *worker= &(test->worker);
  uint32_t counter= 0;
  gearman_return_t rc;

  gearman_worker_set_timeout(worker, 200);
  rc= gearman_worker_add_function(worker, "queue_test", 5, counter_function, &counter);
  test_truth(rc == GEARMAN_SUCCESS);

  // Clean out any jobs that might still be in the queue from failed tests.
  while (1)
  {
    rc= gearman_worker_work(worker);
    if (rc != GEARMAN_SUCCESS)
      break;
  }

  return TEST_SUCCESS;
}

test_return_t queue_add(void *object)
{
  worker_test_st *test= (worker_test_st *)object;
  gearman_client_st client, *client_ptr;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  uint8_t *value= (uint8_t *)"background_test";
  size_t value_length= strlen("background_test");
  gearman_return_t rc;

  test->run_worker= false;

  client_ptr= gearman_client_create(&client);
  test_truth(client_ptr);

  rc= gearman_client_add_server(&client, NULL, WORKER_TEST_PORT);
  test_truth(rc == GEARMAN_SUCCESS);

  rc= gearman_client_echo(&client, (uint8_t *)value, value_length);
  test_truth(rc == GEARMAN_SUCCESS);

  rc= gearman_client_do_background(&client, "queue_test", NULL, value,
                                   value_length, job_handle);
  test_truth(rc == GEARMAN_SUCCESS);

  gearman_client_free(&client);

  test->run_worker= true;

  return TEST_SUCCESS;
}

test_return_t queue_worker(void *object)
{
  worker_test_st *test= (worker_test_st *)object;
  gearman_worker_st *worker= &(test->worker);
  uint32_t counter= 0;
  gearman_return_t rc;

  if (! test->run_worker)
    return TEST_FAILURE;

  rc= gearman_worker_add_function(worker, "queue_test", 5, counter_function, &counter);
  test_truth(rc == GEARMAN_SUCCESS);

  rc= gearman_worker_work(worker);
  test_truth(rc == GEARMAN_SUCCESS);

  test_truth (counter != 0);

  return TEST_SUCCESS;
}


void *world_create(test_return_t *error)
{
  worker_test_st *test;
  const char *argv[2]= { "test_gearmand", "--libsqlite3-db=tests/gearman.sql"};
  pid_t gearmand_pid;

  gearmand_pid= test_gearmand_start(WORKER_TEST_PORT, "libsqlite3", (char **)argv, 2);

  test= malloc(sizeof(worker_test_st));
  if (! test)
  {
    *error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  memset(test, 0, sizeof(worker_test_st));
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

  test->gearmand_pid= gearmand_pid;

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
  {"echo", 0, echo_test },
  {"clean", 0, queue_clean },
  {"add", 0, queue_add },
  {"worker", 0, queue_worker },
  {0, 0, 0}
};

collection_st collection[] ={
#ifdef HAVE_LIBSQLITE3
  {"sqlite queue", 0, 0, tests},
#endif
  {0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
}
