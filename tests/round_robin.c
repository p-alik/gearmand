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
test_return_t queue_add(void *object);
test_return_t queue_worker(void *object);

test_return_t pre(void *object);
test_return_t post(void *object);

void *world_create(test_return_t *error);
test_return_t world_destroy(void *object);

/* append test for worker */
static void *append_function(gearman_job_st *job __attribute__((unused)),
                             void *context, size_t *result_size,
                             gearman_return_t *ret_ptr __attribute__((unused)))
{
  /* this will will set the last char in the context (buffer) to the */
  /* first char of the work */
  char * buf = (char *)context;
  char * work = (char *)gearman_job_workload(job);
  buf += strlen(buf);
  *buf = *work;
  *result_size= 0;
  return NULL;
}

test_return_t queue_add(void *object)
{
  gearman_return_t rc;
  worker_test_st *test= (worker_test_st *)object;
  gearman_client_st client;
  gearman_client_st *client_ptr;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];

  uint8_t *value= (uint8_t *)strdup("0");
  size_t value_length= 1;

  test->run_worker= false;

  client_ptr= gearman_client_create(&client);
  test_truth(client_ptr);

  rc= gearman_client_add_server(&client, NULL, WORKER_TEST_PORT);
    test_truth(GEARMAN_SUCCESS == rc);

  /* send strings "0", "1" ... "9" to alternating between 2 queues */
  /* queue1 = 1,3,5,7,9 */
  /* queue2 = 0,2,4,6,8 */
  for (uint32_t x= 0; x < 10; x++)
  {
    rc= gearman_client_do_background(&client, x % 2 ? "queue1" : "queue2", NULL,
                                     value, value_length, job_handle);

    test_truth(GEARMAN_SUCCESS == rc);
    *value = (uint8_t)(*value + 1);
  }

  gearman_client_free(&client);
  free(value);

  test->run_worker= true;
  return TEST_SUCCESS;
}

test_return_t queue_worker(void *object)
{
  worker_test_st *test= (worker_test_st *)object;
  gearman_worker_st *worker= &(test->worker);
  char buffer[11];
  memset(buffer, 0, sizeof(buffer));

  if (! test->run_worker)
    return TEST_FAILURE;

  if (gearman_worker_add_function(worker, "queue1", 5, append_function,
                                  buffer) != GEARMAN_SUCCESS)
  {
    return TEST_FAILURE;
  }

  if (gearman_worker_add_function(worker, "queue2", 5, append_function,
                                  buffer) != GEARMAN_SUCCESS)
  {
    return TEST_FAILURE;
  }

  for (uint32_t x= 0; x < 10; x++)
  {
    if (gearman_worker_work(worker) != GEARMAN_SUCCESS)
      return TEST_FAILURE;
  }

  // expect buffer to be reassembled in a predictable round robin order
  if( strcmp(buffer, "1032547698") ) 
  {
    fprintf(stderr, "\n\nexpecting 0123456789, got %s\n\n", buffer);
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}


void *world_create(test_return_t *error)
{
  worker_test_st *test;
  const char *argv[2]= { "test_gearmand", "--round-robin"};
  pid_t gearmand_pid;

  gearmand_pid= test_gearmand_start(WORKER_TEST_PORT, NULL, (char **)argv, 2);

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
  gearman_worker_free(&(test->worker));
  test_gearmand_stop(test->gearmand_pid);
  free(test);

  return TEST_SUCCESS;
}

test_st tests[] ={
  {"add", 0, queue_add },
  {"worker", 0, queue_worker },
  {0, 0, 0}
};

collection_st collection[] ={
  {"round_robin", 0, 0, tests},
  {0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
}
