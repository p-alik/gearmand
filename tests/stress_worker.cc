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

static test_return_t pre_recv(void *)
{
  set_recv_close(true, 20, 20);

  return TEST_SUCCESS;
}

static test_return_t post_recv(void *)
{
  set_recv_close(true, 0, 0);

  return TEST_SUCCESS;
}

static test_return_t pre_send(void *)
{
  set_send_close(true, 20, 20);

  return TEST_SUCCESS;
}

static test_return_t post_send(void *)
{
  set_send_close(true, 0, 0);

  return TEST_SUCCESS;
}

/*********************** World functions **************************************/

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  // Assume we are running under valgrind, and bail 
  if (getenv("TESTS_ENVIRONMENT")) 
  {
    error= TEST_SKIPPED;
    return NULL;
  }

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

  gearman_function_t echo_react_fn= gearman_function_create(echo_or_react_worker_v2);

  for (uint32_t x= 0; x < 100; x++)
  {
    if (test_worker_start(libtest::default_port(), NULL, WORKER_FUNCTION_NAME, echo_react_fn, NULL, gearman_worker_options_t()) == NULL)
    {
      error= TEST_FAILURE;
      return NULL;
    }
  }

  return NULL;
}

test_st tests[] ={
  {"first pass", 0, worker_ramp_TEST },
  {"second pass", 0, worker_ramp_TEST },
  {"first pass 1K jobs", 0, worker_ramp_1K_TEST },
  {"first pass 10K jobs", 0, worker_ramp_10K_TEST },
  {0, 0, 0}
};

collection_st collection[] ={
  {"plain", pre_recv, post_recv, tests},
  {"hostile recv()", pre_recv, post_recv, tests},
  {"hostile send()", pre_send, post_send, tests},
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  world->collections= collection;
  world->_create= world_create;
}
