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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <libgearman/gearman.h>

#include <tests/basic.h>
#include <tests/context.h>

#include <tests/ports.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static test_return_t gearmand_basic_option_test(void *)
{
  const char *args[]= { "--queue=libmemcached",  "--libmemcached-servers=localhost:12555", "--check-args", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args));
  return TEST_SUCCESS;
}

static test_return_t collection_init(void *object)
{
  const char *argv[3]= { "test_gearmand", "--libmemcached-servers=localhost:12555", "--queue-type=libmemcached" };

  Context *test= (Context *)object;
  assert(test);

  test_truth(test->initialize(3, argv));

  return TEST_SUCCESS;
}

static test_return_t collection_cleanup(void *object)
{
  Context *test= (Context *)object;
  test->reset();

  return TEST_SUCCESS;
}


static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (has_memcached_support() == false)
  {
    error= TEST_SKIPPED;
    return NULL;
  }

  if (not server_startup(servers, "memcached", 12555, 0, NULL))
  {
    error= TEST_FAILURE;
    return NULL;
  }

  Context *test= new Context(MEMCACHED_TEST_PORT, servers);
  if (not test)
  {
    error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  return test;
}

static bool world_destroy(void *object)
{
  Context *test= (Context *)object;

  delete test;

  return TEST_SUCCESS;
}

test_st gearmand_basic_option_tests[] ={
  {"--queue=libmemcached --libmemcached-servers=", 0, gearmand_basic_option_test },
  {0, 0, 0}
};

test_st tests[] ={
  {"gearman_client_echo()", 0, client_echo_test },
  {"gearman_client_echo() fail", 0, client_echo_fail_test },
  {"gearman_worker_echo()", 0, worker_echo_test },
  {"clean", 0, queue_clean },
  {"add", 0, queue_add },
  {"worker", 0, queue_worker },
  {0, 0, 0}
};

collection_st collection[] ={
  {"gearmand options", 0, 0, gearmand_basic_option_tests},
  {"memcached queue", collection_init, collection_cleanup, tests},
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  world->collections= collection;
  world->_create= world_create;
  world->_destroy= world_destroy;
}
