/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <libtest/common.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <unistd.h>

#include <libgearman/gearman.h>

#include <libtest/server.h>

#include <tests/ports.h>

struct worker_test_st
{
  pid_t gearmand_pid;
  gearman_worker_st worker;
  bool run_worker;

  worker_test_st() :
    gearmand_pid(-1),
    worker(),
    run_worker(false)
    { }
};

/* Prototypes */
void *world_create(test_return_t *error);
test_return_t world_destroy(void *object);

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

/* append test for worker */
static void *append_function(gearman_job_st *job,
                             void *context, size_t *result_size,
                             gearman_return_t *ret_ptr)
{
  /* this will will set the last char in the context (buffer) to the */
  /* first char of the work */
  char *buf = (char *)context;
  assert(buf);

  char *work = (char *)gearman_job_workload(job);
  buf += strlen(buf);
  *buf= *work;
  *result_size= 0;
  *ret_ptr= GEARMAN_SUCCESS;

  return NULL;
}

static test_return_t queue_add(void *object)
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

  test_compare(GEARMAN_SUCCESS,
               gearman_client_add_server(&client, NULL, ROUND_ROBIN_WORKER_TEST_PORT));

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

static test_return_t queue_worker(void *object)
{
  worker_test_st *test= (worker_test_st *)object;
  test_truth(test);

  gearman_worker_st *worker= &(test->worker);
  test_truth(worker);

  char buffer[11];
  memset(buffer, 0, sizeof(buffer));

  test_truth(test->run_worker);

  test_compare_got(GEARMAN_SUCCESS,
                   gearman_worker_add_function(worker, "queue1", 5, append_function, buffer),
                   gearman_worker_error(worker));

  test_compare_got(GEARMAN_SUCCESS,
                   gearman_worker_add_function(worker, "queue2", 5, append_function, buffer),
                   gearman_worker_error(worker));

  for (uint32_t x= 0; x < 10; x++)
  {
    gearman_return_t rc;
    test_compare_got(GEARMAN_SUCCESS,
                     rc= gearman_worker_work(worker),
                     gearman_worker_error(worker) ? gearman_worker_error(worker) : gearman_strerror(rc));
  }

  // expect buffer to be reassembled in a predictable round robin order
  test_strcmp("1032547698", buffer);

  return TEST_SUCCESS;
}


void *world_create(test_return_t *error)
{
  const char *argv[2]= { "test_gearmand", "--round-robin"};
  pid_t gearmand_pid;

  gearmand_pid= test_gearmand_start(ROUND_ROBIN_WORKER_TEST_PORT, 2, argv);

  if (gearmand_pid == -1)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  worker_test_st *test= new (std::nothrow) worker_test_st;;
  if (not test)
  {
    *error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  if (gearman_worker_create(&(test->worker)) == NULL)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  if (gearman_worker_add_server(&(test->worker), NULL, ROUND_ROBIN_WORKER_TEST_PORT) != GEARMAN_SUCCESS)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  test->gearmand_pid= gearmand_pid;

  *error= TEST_SUCCESS;

  return test;
}

test_return_t world_destroy(void *object)
{
  worker_test_st *test= (worker_test_st *)object;
  gearman_worker_free(&(test->worker));
  test_gearmand_stop(test->gearmand_pid);
  delete test;

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

void get_world(Framework *world)
{
  world->collections= collection;
  world->_create= world_create;
  world->_destroy= world_destroy;
}
