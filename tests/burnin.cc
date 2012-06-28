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

#include <tests/burnin.h>

#define DEFAULT_WORKER_NAME "burnin"

static gearman_return_t worker_fn(gearman_job_st*, void*)
{
  return GEARMAN_SUCCESS;
}

struct client_test_st {
  gearman_client_st _client;
  worker_handle_st *handle;

  client_test_st():
    handle(NULL)
  {
    if (gearman_client_create(&_client) == NULL)
    {
      fatal_message("gearman_client_create() failed");
    }

    if (gearman_failed(gearman_client_add_server(&_client, NULL, libtest::default_port())))
    {
      fatal_message("gearman_client_add_server()");
    }

    gearman_function_t func_arg= gearman_function_create(worker_fn);
    handle= test_worker_start(libtest::default_port(), NULL, DEFAULT_WORKER_NAME, func_arg, NULL, gearman_worker_options_t());
  }

  ~client_test_st()
  {
    gearman_client_free(&_client);
    delete handle;
  }

  gearman_client_st* client()
  {
    return &_client;
  }
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

  ~client_context_st()
  {
    if (blob)
    {
      free(blob);
    }
  }
};

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static client_test_st *test_client_context= NULL;
test_return_t burnin_TEST(void *object)
{
  gearman_client_st *client= test_client_context->client();
  fatal_assert(client);

  client_context_st *context= (client_context_st *)gearman_client_context(client);
  fatal_assert(context);

  // This sketchy, don't do this in your own code.
  test_true(context->num_tasks > 0);
  std::vector<gearman_task_st> tasks;
  try {
    tasks.resize(context->num_tasks);
  }
  catch (...)
  { }
  test_compare(tasks.size(), context->num_tasks);

  test_compare(gearman_client_echo(client, test_literal_param("echo_test")), GEARMAN_SUCCESS);

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
        {
          blob_size*= (size_t)(rand() + 1);
        }

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

      test_compare(ret, GEARMAN_SUCCESS);
      test_truth(task_ptr);
    }

    gearman_return_t ret= gearman_client_run_tasks(client);
    for (uint32_t x= 0; x < context->num_tasks; x++)
    {
      test_compare(GEARMAN_TASK_STATE_FINISHED, tasks[x].state);
      test_compare(GEARMAN_SUCCESS, tasks[x].result_rc);
    }
    test_zero(client->new_tasks);

    test_compare(ret, GEARMAN_SUCCESS);

    for (uint32_t x= 0; x < context->num_tasks; x++)
    {
      gearman_task_free(&(tasks[x]));
    }
  } while (context->count--);

  context->latch++;

  return TEST_SUCCESS;
}

test_return_t burnin_setup(void *object)
{
  test_client_context= new client_test_st;
  client_context_st *context= new client_context_st;

  context->blob= (char *)malloc(context->max_size);
  test_true(context->blob);
  memset(context->blob, 'x', context->max_size); 

  gearman_client_set_context(test_client_context->client(), context);

  return TEST_SUCCESS;
}

test_return_t burnin_cleanup(void *object)
{
  client_context_st *context= (struct client_context_st *)gearman_client_context(test_client_context->client());

  delete context;
  delete test_client_context;
  test_client_context= NULL;

  return TEST_SUCCESS;
}
