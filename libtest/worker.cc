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

#include <libtest/test.hpp>
#include <libtest/worker.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

struct context_st {
  in_port_t port;
  const char *function_name;
  struct worker_handle_st *handle;
  gearman_worker_options_t options;
  gearman_function_t& worker_fn;
  const char *namespace_key;
  void *context;

  context_st(gearman_function_t& arg) :
    port(0),
    handle(0),
    options(gearman_worker_options_t()),
    worker_fn(arg),
    namespace_key(NULL),
    context(0)
  { }
};

static void *thread_runner(void *con)
{
  struct context_st *context= (struct context_st *)con;

  assert(context);

  gearman_worker_st *worker= gearman_worker_create(NULL);
  assert(worker);

  if (context->namespace_key)
  {
    gearman_worker_set_namespace(worker, context->namespace_key, strlen(context->namespace_key));
  }

  gearman_return_t rc= gearman_worker_add_server(worker, NULL, context->port);
  assert(rc == GEARMAN_SUCCESS);

  bool success= gearman_worker_set_server_option(worker, gearman_literal_param("exceptions"));
  assert(success);

  rc= gearman_worker_define_function(worker,
				     context->function_name, strlen(context->function_name), 
				     context->worker_fn,
				     0, 
				     context->context);
  assert(rc == GEARMAN_SUCCESS);

  if (context->options != gearman_worker_options_t())
  {
    gearman_worker_add_options(worker, context->options);
  }

  gearman_worker_set_timeout(worker, 100);

  assert(context->handle);

  while (context->handle->shutdown == false)
  {
    gearman_return_t ret= gearman_worker_work(worker);
    (void)ret;
  }

  gearman_worker_free(worker);

  free(context);

  pthread_exit(0);
}

struct worker_handle_st *test_worker_start(in_port_t port, 
					   const char *namespace_key,
					   const char *function_name,
					   gearman_function_t &worker_fn,
					   void *context,
					   gearman_worker_options_t options)
{
  pthread_attr_t attr;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  worker_handle_st *handle= new worker_handle_st();
  assert(handle);

  context_st *foo= new context_st(worker_fn);

  foo->port= port;
  foo->function_name= function_name;
  foo->context= context;
  foo->handle= handle;
  foo->options= options;
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

void test_worker_stop(struct worker_handle_st *handle)
{
  void *unused;
  fflush(stderr);
  handle->shutdown= true;
  pthread_join(handle->thread, &unused);
  free(handle);
}
