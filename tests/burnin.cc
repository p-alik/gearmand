/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <config.h>
#include <libtest/test.hpp>

using namespace libtest;

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <libgearman/gearman.h>

#include <libtest/test.hpp>

#include <tests/start_worker.h>

#define DEFAULT_WORKER_NAME "burnin"

struct client_test_st {
  gearman_client_st client;
  pid_t gearmand_pid;
  struct worker_handle_st *handle;

  client_test_st():
    gearmand_pid(-1),
    handle(NULL)
  { }
};

struct client_context_st {
  int latch;
  size_t min_size;
  size_t max_size;
  size_t num_tasks;
  size_t count;
  char *blob;

  client_context_st():
    latch(0),
    min_size(1024),
    max_size(1024 *2),
    num_tasks(20),
    count(2000),
    blob(NULL)
  { }
};

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static test_return_t burnin_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  struct client_context_st *context= (struct client_context_st *)gearman_client_context(client);

  // This sketchy, don't do this in your own code.
  gearman_task_st *tasks= (gearman_task_st *)calloc(context->num_tasks, sizeof(gearman_task_st));
  test_true_got(tasks, strerror(errno));

  test_true_got(gearman_success(gearman_client_echo(client, test_literal_param("echo_test"))), gearman_client_error(client));

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

      gearman_task_st *task_ptr;
      gearman_return_t ret;
      if (context->latch)
      {
        task_ptr= gearman_client_add_task_background(client, &(tasks[x]),
                                                     NULL, DEFAULT_WORKER_NAME, NULL,
                                                     (void *)context->blob, blob_size, &ret);
      }
      else
      {
        task_ptr= gearman_client_add_task(client, &(tasks[x]), NULL,
                                          DEFAULT_WORKER_NAME, NULL, (void *)context->blob, blob_size,
                                          &ret);
      }

      test_true_got(gearman_success(ret), gearman_client_error(client));
      test_truth(task_ptr);
    }

    gearman_return_t ret= gearman_client_run_tasks(client);
    for (uint32_t x= 0; x < context->num_tasks; x++)
    {
      test_compare(GEARMAN_TASK_STATE_FINISHED, tasks[x].state);
      test_compare(GEARMAN_SUCCESS, tasks[x].result_rc);
    }
    test_zero(client->new_tasks);

    test_true_got(gearman_success(ret), gearman_client_error(client));

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

  struct client_context_st *context= new client_context_st;
  test_true_got(context, strerror(errno));

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
  delete(context);

  return TEST_SUCCESS;
}


static void *worker_fn(gearman_job_st *, void *,
                       size_t *result_size, gearman_return_t *ret_ptr)
{
  *result_size= 0;
  *ret_ptr= GEARMAN_SUCCESS;
  return NULL;
}

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  /**
   *  @TODO We cast this to char ** below, which is evil. We need to do the
   *  right thing
   */
  const char *argv[1]= { "client_gearmand" };

  client_test_st *test= new client_test_st;
  if (not test)
  {
    error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  /**
    We start up everything before we allocate so that we don't have to track memory in the forked process.
  */
  if (server_startup(servers, "gearmand", libtest::default_port(), 1, argv) == false)
  {
    error= TEST_FAILURE;
    return NULL;
  }

  gearman_function_t func_arg= gearman_function_create_v1(worker_fn);
  test->handle= test_worker_start(libtest::default_port(), NULL, DEFAULT_WORKER_NAME, func_arg, NULL, gearman_worker_options_t());
  if (not test->handle)
  {
    error= TEST_FAILURE;
    return NULL;
  }

  if (not gearman_client_create(&(test->client)))
  {
    error= TEST_FAILURE;
    return NULL;
  }

  if (gearman_failed(gearman_client_add_server(&(test->client), NULL, libtest::default_port())))
  {
    error= TEST_FAILURE;
    return NULL;
  }

  error= TEST_SUCCESS;

  return (void *)test;
}

static bool world_destroy(void *object)
{
  client_test_st *test= (client_test_st *)object;
  gearman_client_free(&(test->client));
  delete test->handle;

  delete test;

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
class GearmandRunner : public Runner {
public:
  test_return_t run(test_callback_fn* func, void *object)
  {
    if (func)
    {
      libgearman_test_callback_fn actual= libgearman_test_callback_fn(func);
      client_test_st *container= (client_test_st*)object;

      return actual(&container->client);
    }

    return TEST_SUCCESS;
  }

  test_return_t pre(test_callback_fn* func, void *object)
  {
    if (func)
    {
      libgearman_test_callback_fn actual= libgearman_test_callback_fn(func);
      client_test_st *container= (client_test_st*)object;


      return actual(&container->client);
    }

    return TEST_SUCCESS;
  }

  test_return_t post(test_callback_fn* func, void *object)
  {
    if (func)
    {
      libgearman_test_callback_fn actual= libgearman_test_callback_fn(func);
      client_test_st *container= (client_test_st*)object;

      return actual(&container->client);
    }

    return TEST_SUCCESS;
  }
};

static GearmandRunner defualt_runner;

void get_world(Framework *world)
{
  world->collections= collection;
  world->_create= world_create;
  world->_destroy= world_destroy;
  world->set_runner(&defualt_runner);
}
