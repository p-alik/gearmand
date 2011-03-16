/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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

#include "libtest/test.h"
#include "libtest/server.h"
#include "tests/test_worker.h"

#define WORKER_TEST_PORT 32123
#define WORKER_FUNCTION  "queue_test"

typedef struct
{
  pid_t gearmand_pid;
} local_test_st;

/* Prototypes */
test_return_t pre(void *object);
test_return_t post(void *object);

void *world_create(test_return_t *error);
test_return_t world_destroy(void *object);

pthread_mutex_t counter_lock= PTHREAD_MUTEX_INITIALIZER;
/* Counter test for worker */
static void *counter_function(gearman_job_st *job __attribute__((unused)),
                              void *context, size_t *result_size,
                              gearman_return_t *ret_ptr __attribute__((unused)))
{
  uint32_t *counter= (uint32_t *)context;

  *result_size= 0;

  pthread_mutex_lock(&counter_lock);
  *counter= *counter + 1;
  pthread_mutex_unlock(&counter_lock);

  return NULL;
}

#define NUMBER_OF_WORKERS 10
#define NUMBER_OF_JOBS 40
#define JOB_SIZE 100
static test_return_t lp_734663(void *object)
{
  (void)object;

  gearman_client_st client, *client_ptr;
  char job_handle[GEARMAN_JOB_HANDLE_SIZE];
  uint8_t value[JOB_SIZE];
  gearman_return_t rc;

  memset(&value, 'x', sizeof(value));

  client_ptr= gearman_client_create(&client);
  test_truth(client_ptr);

  rc= gearman_client_add_server(&client, NULL, WORKER_TEST_PORT);
  test_truth(rc == GEARMAN_SUCCESS);

  rc= gearman_client_echo(&client, value, sizeof(JOB_SIZE));
  test_truth(rc == GEARMAN_SUCCESS);

  for (uint32_t x= 0; x < NUMBER_OF_JOBS; x++)
  {
    rc= gearman_client_do_background(&client, WORKER_FUNCTION, NULL, value,
                                     sizeof(value), job_handle);
    test_truth(rc == GEARMAN_SUCCESS);
  }

  gearman_client_free(&client);

  struct worker_handle_st *worker_handle[NUMBER_OF_WORKERS];

  uint32_t counter= 0;
  for (uint32_t x= 0; x < NUMBER_OF_WORKERS; x++)
  {
    worker_handle[x]= test_worker_start(WORKER_TEST_PORT, WORKER_FUNCTION, counter_function, &counter);
  }

  time_t end_time= time(NULL) + 5;
  time_t current_time= 0;
  while (counter < NUMBER_OF_JOBS || current_time < end_time)
  {
    sleep(1);
    current_time= time(NULL);
  }

  for (uint32_t x= 0; x < NUMBER_OF_WORKERS; x++)
  {
    test_worker_stop(worker_handle[x]);
  }


  return TEST_SUCCESS;
}


void *world_create(test_return_t *error)
{
  local_test_st *test;
  const char *argv[2]= { "test_gearmand", "--libsqlite3-db=tests/gearman.sql"};
  pid_t gearmand_pid;

  unlink("tests/gearman.sql");
  unlink("tests/gearman.sql-journal");

  gearmand_pid= test_gearmand_start(WORKER_TEST_PORT, "libsqlite3", (char **)argv, 2);

  test= malloc(sizeof(local_test_st));
  if (! test)
  {
    *error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  memset(test, 0, sizeof(local_test_st));
  test->gearmand_pid= gearmand_pid;

  *error= TEST_SUCCESS;

  return (void *)test;
}

test_return_t world_destroy(void *object)
{
  local_test_st *test= (local_test_st *)object;
  test_gearmand_stop(test->gearmand_pid);
  free(test);

  unlink("tests/gearman.sql");
  unlink("tests/gearman.sql-journal");

  return TEST_SUCCESS;
}

test_st tests[] ={
  {"lp:734663", 0, lp_734663 },
  {0, 0, 0}
};

collection_st collection[] ={
  {"queue regressions", 0, 0, tests},
  {0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
}
