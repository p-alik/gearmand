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
#include <errno.h>

#include <libgearman/gearman.h>

#include <libtest/test.h>
#include <libtest/server.h>
#include <libtest/worker.h>

#define CLIENT_TEST_PORT 32123

#define DEFAULT_WORKER_NAME "burnin"

typedef struct
{
  gearman_client_st client;
  pid_t gearmand_pid;
  struct worker_handle_st *handle;
} client_test_st;

struct client_context_st {
  int latch;
  size_t min_size;
  size_t max_size;
  size_t num_tasks;
  size_t count;
  char *blob;
};

void *world_create(test_return_t *error);
test_return_t world_destroy(void *object);

#pragma GCC diagnostic ignored "-Wold-style-cast"

static test_return_t burnin_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  struct client_context_st *context= (struct client_context_st *)gearman_client_context(client);

  gearman_task_st *tasks= (gearman_task_st *)calloc(context->num_tasks, sizeof(gearman_task_st));
  test_true_got(tasks, strerror(errno));

  do
  {
    for (uint32_t x= 0; x < context->num_tasks; x++)
    {
      size_t blob_size= 0;

      if (context->min_size == context->max_size)
      {
        blob_size= context->max_size;
      }
      else
      {
        blob_size= (size_t)rand();

        if (context->max_size > RAND_MAX)
          blob_size*= (size_t)(rand() + 1);

        blob_size= (blob_size % (context->max_size - context->min_size)) + context->min_size;
      }

      gearman_return_t ret;
      if (context->latch)
      {
        (void)gearman_client_add_task_background(client, &(tasks[x]),
                                                 NULL, DEFAULT_WORKER_NAME, NULL,
                                                 (void *)context->blob, blob_size, &ret);
      }
      else
      {
        (void)gearman_client_add_task(client, &(tasks[x]), NULL,
                                      DEFAULT_WORKER_NAME, NULL, (void *)context->blob, blob_size,
                                      &ret);
      }

      if (ret != GEARMAN_SUCCESS)
      {
        if (ret == GEARMAN_LOST_CONNECTION)
          continue;

        test_true_got(false, gearman_client_error(client));
      }
    }

    gearman_return_t ret;
    ret= gearman_client_run_tasks(client);

    if (ret != GEARMAN_SUCCESS && ret != GEARMAN_LOST_CONNECTION)
    {
      test_true_got(false, gearman_client_error(client));
    }

    for (uint32_t x= 0; x < context->num_tasks; x++)
    {
      gearman_task_free(&(tasks[x]));
    }

  } while (context->count--);

  free(tasks);

  context->latch++;

  return TEST_SUCCESS;
}

static test_return_t setup(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  struct client_context_st *context= (struct client_context_st *)calloc(1, sizeof(struct client_context_st));
  test_true_got(context, strerror(errno));

  context->min_size= 1024;
  context->max_size= context->min_size *2;
  context->num_tasks= 10;
  context->count= 1000;

  context->blob= (char *)malloc(context->max_size);
  test_true_got(context->blob, strerror(errno));
  memset(context->blob, 'x', context->max_size); 

  gearman_client_set_context(client, context);

  return TEST_SUCCESS;
}

static test_return_t cleanup(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  struct client_context_st *context= (struct client_context_st *)gearman_client_context(client);

  free(context->blob);

  return TEST_SUCCESS;
}


static void *worker_fn(gearman_job_st *job, void *context,
                       size_t *result_size, gearman_return_t *ret_ptr)
{
  (void)job;
  (void)context;
  (void)result_size;

  *ret_ptr= GEARMAN_SUCCESS;
  return NULL;
}

void *world_create(test_return_t *error)
{
  client_test_st *test;
  pid_t gearmand_pid;

  /**
   *  @TODO We cast this to char ** below, which is evil. We need to do the
   *  right thing
   */
  const char *argv[1]= { "client_gearmand" };

  test= (client_test_st *)calloc(1, sizeof(client_test_st));
  if (! test)
  {
    *error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  /**
    We start up everything before we allocate so that we don't have to track memory in the forked process.
  */
  gearmand_pid= test_gearmand_start(CLIENT_TEST_PORT, NULL,
                                    (char **)argv, 1);
  test->handle= test_worker_start(CLIENT_TEST_PORT, "burnin", worker_fn, NULL);

  test->gearmand_pid= gearmand_pid;

  if (gearman_client_create(&(test->client)) == NULL)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  if (gearman_client_add_server(&(test->client), NULL, CLIENT_TEST_PORT) != GEARMAN_SUCCESS)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  *error= TEST_SUCCESS;

  return (void *)test;
}

test_return_t world_destroy(void *object)
{
  client_test_st *test= (client_test_st *)object;
  gearman_client_free(&(test->client));
  test_gearmand_stop(test->gearmand_pid);
  test_worker_stop(test->handle);
  free(test);

  return TEST_SUCCESS;
}


test_st tests[] ={
  {"burnin", 0, burnin_test },
//  {"burnin_background", 0, burnin_test },
  {0, 0, 0}
};


collection_st collection[] ={
  {"burnin", setup, cleanup, tests},
  {0, 0, 0, 0}
};

typedef test_return_t (*libgearman_test_callback_fn)(gearman_client_st *);
static test_return_t _runner_default(libgearman_test_callback_fn func, client_test_st *container)
{
  if (func)
  {
    return func(&container->client);
  }
  else
  {
    return TEST_SUCCESS;
  }
}

static world_runner_st runner= {
  (test_callback_runner_fn)_runner_default,
  (test_callback_runner_fn)_runner_default,
  (test_callback_runner_fn)_runner_default
};


void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
  world->runner= &runner;
}
