/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include <libtest/common.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libgearman/gearman.h>

#include <tests/basic.h>
#include <tests/context.h>

#include <tests/ports.h>

#define WORKER_FUNCTION "drizzle_queue_test"

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static test_return_t collection_init(void *object)
{
  Context *test= (Context *)object;
  assert(test);

  const char *argv[2]= { "test_gearmand", "--queue-type=libdrizzle" };

  test->initialize(2, argv);

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
  (void)collection_init;
  (void)collection_cleanup;

  Context *test= new Context(DRIZZLE_TEST_PORT, servers);
  if (not test)
  {
    error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  error= TEST_SUCCESS;

  return test;
}

static bool world_destroy(void *object)
{
  Context *test= (Context *)object;

  delete test;

  return TEST_SUCCESS;
}

test_st tests[] ={
  {"gearman_client_echo()", 0, client_echo_test },
  {"gearman_client_echo() fail", 0, client_echo_fail_test },
  {"gearman_worker_echo()", 0, worker_echo_test },
  {"clean", 0, queue_clean },
  {"add", 0, queue_add },
  {"worker", 0, queue_worker },
  {0, 0, 0}
};

test_st regressions[] ={
  {"lp:734663", 0, lp_734663 },
  {0, 0, 0}
};

collection_st collection[] ={
#ifdef HAVE_LIBDRIZZLE
  {"drizzle queue", collection_init, collection_cleanup, tests},
  {"regressions", collection_init, collection_cleanup, regressions},
#endif
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  world->collections= collection;
  world->_create= world_create;
  world->_destroy= world_destroy;
}
