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
#include <unistd.h>

#include <libgearman/gearman.h>

#include <libhostile/hostile.h>

#include <tests/ports.h>
#include <tests/start_worker.h>
#include <tests/workers.h>

#define WORKER_FUNCTION_NAME "foo"

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
    size_t *success= (size_t *)object;
    assert(*success == 0);

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
      rc= gearman_client_add_server(client, NULL, STRESS_WORKER_PORT);
      pthread_setcanceltype(oldstate, NULL);
    }

    pthread_cleanup_push(client_cleanup, client);

    if (gearman_success(rc))
    {
      gearman_client_set_timeout(client, 400);
      for (size_t x= 0; x < 10; x++)
      {
        int oldstate;
        pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, &oldstate);
        (void)gearman_client_do(client, WORKER_FUNCTION_NAME, NULL, NULL, 0, NULL, &rc);
        pthread_setcanceltype(oldstate, NULL);

        if (gearman_success(rc))
        {
          *success= *success +1;
        }
      }
    }

    pthread_cleanup_pop(1);
    pthread_exit(0);
  }
}

#define CLIENT_CHILDREN 100
static test_return_t worker_ramp_TEST(void *)
{
  std::vector<pthread_t> children;
  children.resize(CLIENT_CHILDREN);

  std::vector<size_t>  success;
  success.resize(children.size());

  set_recv_close(true, 20, 20);

  for (size_t x= 0; x < children.size(); x++)
  {
    pthread_create(&children[x], NULL, client_thread, &success[x]);
  }
  
  for (size_t x= 0; x < children.size(); x++)
  {
#ifdef _GNU_SOURCE
    {
      struct timespec ts;

      if (clock_gettime(CLOCK_REALTIME, &ts) == -1) 
      {
        Error << strerror(errno);
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

  set_recv_close(true, 0, 0);

  return TEST_SUCCESS;
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

  if (server_startup(servers, "gearmand", STRESS_WORKER_PORT, 0, NULL) == false)
  {
    error= TEST_FAILURE;
    return NULL;
  }

  gearman_function_t echo_react_fn= gearman_function_create(echo_or_react_worker_v2);

  for (uint32_t x= 0; x < 100; x++)
  {
    test_worker_start(STRESS_WORKER_PORT, NULL, WORKER_FUNCTION_NAME, echo_react_fn, NULL, gearman_worker_options_t());
  }

  return NULL;
}

static bool world_destroy(void *)
{
  return TEST_SUCCESS;
}

test_st tests[] ={
  {"first pass", 0, worker_ramp_TEST },
  {"second pass", 0, worker_ramp_TEST },
  {0, 0, 0}
};

collection_st collection[] ={
  {"recv()", pre_recv, post_recv, tests},
  {"send()", pre_send, post_send, tests},
  {0, 0, 0, 0}
};


typedef test_return_t (*libgearman_test_prepost_callback_fn)(void *);
typedef test_return_t (*libgearman_test_callback_fn)(gearman_worker_st *);
static test_return_t _runner_prepost_default(libgearman_test_prepost_callback_fn func, void *)
{
  if (func)
  {
    return func(NULL);
  }

  return TEST_SUCCESS;
}

static test_return_t _runner_default(libgearman_test_callback_fn func, void *)
{
  if (func)
  {
    return func(NULL);
  }

  return TEST_SUCCESS;
}

class GearmandRunner : public Runner {
public:
  test_return_t run(test_callback_fn* func, void *object)
  {
    return _runner_default(libgearman_test_callback_fn(func), object);
  }

  test_return_t pre(test_callback_fn* func, void *object)
  {
    return _runner_prepost_default(libgearman_test_prepost_callback_fn(func), object);
  }

  test_return_t post(test_callback_fn* func, void *object)
  {
    return _runner_prepost_default(libgearman_test_prepost_callback_fn(func), object);
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
