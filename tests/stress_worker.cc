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

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>

#include <libgearman/gearman.h>

#include <libhostile/hostile.h>

#include <tests/start_worker.h>
#include <tests/workers.h>
#include "tests/burnin.h"

#define WORKER_FUNCTION_NAME "foo"

struct client_thread_context_st
{
  size_t count;
  size_t payload_size;

  client_thread_context_st() :
    count(0),
    payload_size(0)
  { }

  void increment()
  {
    count++;
  }
};

extern "C" {

  static void client_cleanup(void *client)
  {
    int oldstate;
    pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, &oldstate);
    gearman_client_free((gearman_client_st *)client);
    pthread_setcanceltype(oldstate, NULL);
  }

  static void *client_thread(void *object)
  {
    client_thread_context_st *success= (client_thread_context_st *)object;
    fatal_assert(success);
    fatal_assert(success->count == 0);

    gearman_return_t rc;
    gearman_client_st *client;
    {
      int oldstate;
      pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, &oldstate);
      client= gearman_client_create(NULL);

      if (client == NULL)
      {
        pthread_exit(0);
      }
      rc= gearman_client_add_server(client, NULL, libtest::default_port());
      pthread_setcanceltype(oldstate, NULL);
    }

    pthread_cleanup_push(client_cleanup, client);

    if (gearman_success(rc))
    {
      gearman_client_set_timeout(client, 400);
      for (size_t x= 0; x < 100; x++)
      {
        int oldstate;
        pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, &oldstate);
        libtest::vchar_t payload;
        payload.resize(success->payload_size);
        void *value= gearman_client_do(client, WORKER_FUNCTION_NAME,
                                       NULL,
                                       &payload[0], payload.size(),
                                       NULL, &rc);
        pthread_setcanceltype(oldstate, NULL);

        if (gearman_success(rc))
        {
          success->increment();
        }

        if (value)
        {
          free(value);
        }
      }
    }

    pthread_cleanup_pop(1);
    pthread_exit(0);
  }
}

static test_return_t worker_ramp_exec(const size_t payload_size)
{
  std::vector<pthread_t> children;
  children.resize(number_of_cpus());

  std::vector<client_thread_context_st>  success;
  success.resize(children.size());

  for (size_t x= 0; x < children.size(); x++)
  {
    success[x].payload_size= payload_size;
    pthread_create(&children[x], NULL, client_thread, &success[x]);
  }
  
  for (size_t x= 0; x < children.size(); x++)
  {
#if _GNU_SOURCE && defined(TARGET_OS_LINUX) && TARGET_OS_LINUX 
    {
      struct timespec ts;

      if (HAVE_LIBRT)
      {
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) 
        {
          Error << strerror(errno);
        }
      }
      else
      {
        struct timeval tv;
        if (gettimeofday(&tv, NULL) == -1) 
        {
          Error << strerror(errno);
        }
        TIMEVAL_TO_TIMESPEC(&tv, &ts);
      }

      ts.tv_sec+= 10;

      int error= pthread_timedjoin_np(children[x], NULL, &ts);
      if (error != 0)
      {
        pthread_cancel(children[x]);
        pthread_join(children[x], NULL);
      }
    }
#else
    pthread_join(children[x], NULL);
#endif
  }

  return TEST_SUCCESS;
}

static test_return_t worker_ramp_TEST(void *)
{
  return worker_ramp_exec(0);
}

static test_return_t worker_ramp_1K_TEST(void *)
{
  return worker_ramp_exec(1024);
}

static test_return_t worker_ramp_10K_TEST(void *)
{
  return worker_ramp_exec(1024*10);
}

static test_return_t worker_ramp_SETUP(void *object)
{
  worker_handles_st *handles= (worker_handles_st*)object;

  gearman_function_t echo_react_fn= gearman_function_create(echo_or_react_worker_v2);
  for (uint32_t x= 0; x < 10; x++)
  {
    worker_handle_st *worker;
    if ((worker= test_worker_start(libtest::default_port(), NULL, WORKER_FUNCTION_NAME, echo_react_fn, NULL, gearman_worker_options_t())) == NULL)
    {
      return TEST_FAILURE;
    }
    handles->push(worker);
  }

  return TEST_SUCCESS;
}

static test_return_t worker_ramp_TEARDOWN(void* object)
{
  worker_handles_st *handles= (worker_handles_st*)object;
  handles->reset();

  return TEST_SUCCESS;
}

static test_return_t recv_SETUP(void* object)
{
  test_skip_valgrind();

  worker_ramp_SETUP(object);
  set_recv_close(true, 20, 20);

  return TEST_SUCCESS;
}

static test_return_t resv_TEARDOWN(void* object)
{
  set_recv_close(true, 0, 0);

  worker_handles_st *handles= (worker_handles_st*)object;
  handles->kill_all();

  return TEST_SUCCESS;
}

static test_return_t send_SETUP(void* object)
{
  test_skip_valgrind();

  worker_ramp_SETUP(object);
  set_send_close(true, 20, 20);

  return TEST_SUCCESS;
}

static test_return_t send_TEARDOWN(void* object)
{
  set_send_close(true, 0, 0);

  worker_handles_st *handles= (worker_handles_st*)object;
  handles->kill_all();

  return TEST_SUCCESS;
}


/*********************** World functions **************************************/

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (bool(getenv("YATL_RUN_MASSIVE_TESTS")) == false) 
  {
    error= TEST_SKIPPED;
    return NULL;
  }

  if (server_startup(servers, "gearmand", libtest::default_port(), 0, NULL) == false)
  {
    error= TEST_FAILURE;
    return NULL;
  }

  return new worker_handles_st;
}

static bool world_destroy(void *object)
{
  worker_handles_st *handles= (worker_handles_st *)object;
  delete handles;

  return TEST_SUCCESS;
}

test_st burnin_TESTS[] ={
  {"burnin", 0, burnin_TEST },
  {0, 0, 0}
};


test_st worker_TESTS[] ={
  {"first pass", 0, worker_ramp_TEST },
  {"second pass", 0, worker_ramp_TEST },
  {"first pass 1K jobs", 0, worker_ramp_1K_TEST },
  {"first pass 10K jobs", 0, worker_ramp_10K_TEST },
  {0, 0, 0}
};

collection_st collection[] ={
  {"burnin", burnin_setup, burnin_cleanup, burnin_TESTS },
  {"plain", worker_ramp_SETUP, worker_ramp_TEARDOWN, worker_TESTS },
  {"hostile recv()", recv_SETUP, resv_TEARDOWN, worker_TESTS },
  {"hostile send()", send_SETUP, send_TEARDOWN, worker_TESTS },
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  world->collections= collection;
  world->_create= world_create;
  world->_destroy= world_destroy;
}
