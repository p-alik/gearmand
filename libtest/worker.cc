/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <config.h>

#include <cassert>
#include <cstring>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#include <cstdio>

#include <libtest/test.h>
#include <libtest/worker.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

struct context_st {
  in_port_t port;
  const char *function_name;
  void *function_arg;
  struct worker_handle_st *handle;
  gearman_worker_options_t options;
  gearman_worker_fn *worker_fn;
  gearman_function_fn *partition_fn;
  gearman_aggregator_fn *aggregator_fn;
  const char *namespace_key;

  context_st() :
    port(0),
    function_arg(0),
    handle(0),
    options(gearman_worker_options_t()),
    partition_fn(0),
    aggregator_fn(0),
    namespace_key(NULL)
  { }
};

static void *thread_runner(void *con)
{
  struct context_st *context= (struct context_st *)con;
  gearman_worker_st worker, *worker_ptr;

  worker_ptr= gearman_worker_create(&worker);
  assert(worker_ptr);

  if (context->namespace_key)
    gearman_worker_set_namespace(worker_ptr, context->namespace_key, strlen(context->namespace_key));

  gearman_return_t rc;
  rc= gearman_worker_add_server(&worker, NULL, context->port);
  assert(rc == GEARMAN_SUCCESS);

  bool success= gearman_worker_set_server_option(&worker, gearman_literal_param("exceptions"));
  assert(success);

  if (context->aggregator_fn)
  {
    gearman_function_t function= gearman_function_create_partition(context->partition_fn, context->aggregator_fn);
    rc= gearman_worker_define_function(&worker,
                                       context->function_name, strlen(context->function_name), 
                                       function,
                                       0, 
                                       context->function_arg);
  }
  else
  {
    rc= gearman_worker_add_function(&worker, context->function_name, 0, context->worker_fn,
                                    context->function_arg);
  }
  assert(rc == GEARMAN_SUCCESS);

  if (context->options != gearman_worker_options_t())
  {
    gearman_worker_add_options(&worker, context->options);
  }

  gearman_worker_set_timeout(&worker, 100);

  assert(context->handle);

  while (context->handle->shutdown == false)
  {
    gearman_return_t ret= gearman_worker_work(&worker);
    (void)ret;
  }

  gearman_worker_free(&worker);

  free(context);

  pthread_exit(0);
}

static struct worker_handle_st *_test_worker_start(in_port_t port, 
                                                   const char *namespace_key,
                                                   const char *function_name,
                                                   gearman_worker_fn *worker_fn,
                                                   gearman_function_fn *partition_fn,
                                                   gearman_aggregator_fn *aggregator_fn,
                                                   void *function_arg,
                                                   gearman_worker_options_t options)
{
  pthread_attr_t attr;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  struct worker_handle_st *handle= (struct worker_handle_st *)calloc(1, sizeof(struct worker_handle_st));
  assert(handle);

  struct context_st *foo= (struct context_st *)calloc(1, sizeof(struct context_st));

  foo->port= port;
  foo->function_name= function_name;
  foo->worker_fn= worker_fn;
  foo->function_arg= function_arg;
  foo->handle= handle;
  foo->options= options;
  foo->partition_fn= partition_fn;
  foo->aggregator_fn= aggregator_fn;
  foo->namespace_key= namespace_key;

  test_assert_errno(pthread_create(&handle->thread, &attr, thread_runner, foo));

  pthread_attr_destroy(&attr);

  /* Wait for the server to start and bind the port. */
  struct timespec dream, rem;

  dream.tv_nsec= 0;
  dream.tv_sec= 1;

  nanosleep(&dream, &rem);

  return handle;
}

struct worker_handle_st *test_worker_start(in_port_t port,
                                           const char *function_name,
                                           gearman_worker_fn *worker_fn, 
                                           void *function_arg, gearman_worker_options_t options)
{
  return _test_worker_start(port, NULL, function_name, worker_fn, NULL, NULL, function_arg, options);
}

struct worker_handle_st *test_worker_start_with_namespace(in_port_t port,
                                           const char *function_name,
                                           gearman_worker_fn *worker_fn, 
                                           void *function_arg,
                                           const char *namespace_key,
                                           gearman_worker_options_t options)
{
  return _test_worker_start(port, namespace_key, function_name, worker_fn, NULL, NULL, function_arg, options);
}

struct worker_handle_st *test_worker_start_with_reducer(in_port_t port,
                                                        const char *namespace_key,
                                                        const char *function_name,
                                                        gearman_function_fn *partition_fn, gearman_aggregator_fn *aggregator_fn,  
                                                        void *function_arg,
                                                        gearman_worker_options_t options)
{
  return _test_worker_start(port, namespace_key, function_name, NULL, partition_fn, aggregator_fn, function_arg, options);
}

void test_worker_stop(struct worker_handle_st *handle)
{
  void *unused;
  fflush(stderr);
  handle->shutdown= true;
  pthread_join(handle->thread, &unused);
  free(handle);
}
