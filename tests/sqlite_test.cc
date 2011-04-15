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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>

#include <libgearman/gearman.h>

#include <libtest/test.h>
#include <libtest/server.h>

#include <tests/basic.h>
#include <tests/context.h>

#define WORKER_TEST_PORT 32123

// Prototypes
test_return_t pre(void *object);
test_return_t post(void *object);

void *world_create(test_return_t *error);
test_return_t world_destroy(void *object);

#pragma GCC diagnostic ignored "-Wold-style-cast"

static test_return_t collection_init(void *object)
{
  const char *argv[2]= { "test_gearmand", "--libsqlite3-db=tests/gearman.sql"};

  Context *test= (Context *)object;
  assert(test);

  if (not test->initialize(2, argv))
    return TEST_FAILURE;

  return TEST_SUCCESS;
}

static test_return_t collection_cleanup(void *object)
{
  Context *test= (Context *)object;
  test->reset();

  return TEST_SUCCESS;
}


void *world_create(test_return_t *error)
{
  Context *test= new Context(WORKER_TEST_PORT);
  if (not test)
  {
    *error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  *error= TEST_SUCCESS;

  return test;
}

test_return_t world_destroy(void *object)
{
  Context *test= (Context *)object;

  delete test;

  return TEST_SUCCESS;
}

test_st tests[] ={
  {"echo", 0, echo_test },
  {"clean", 0, queue_clean },
  {"add", 0, queue_add },
  {"worker", 0, queue_worker },
  {0, 0, 0}
};

collection_st collection[] ={
  {"sqlite queue", collection_init, collection_cleanup, tests},
#if 0
  {"sqlite queue change table", collection_init, collection_cleanup, tests},
#endif
  {0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
}
