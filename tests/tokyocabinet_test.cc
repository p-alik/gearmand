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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libgearman/gearman.h>

#include <tests/basic.h>
#include <tests/context.h>

#include <tests/ports.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static test_return_t test_for_HAVE_LIBTOKYOCABINET(void *)
{
#ifdef HAVE_LIBSQLITE3
  return TEST_SUCCESS;
#else
  return TEST_SKIPPED;
#endif
}

static test_return_t gearmand_basic_option_test(void *)
{
  const char *args[]= { "--queue=libtokyocabinet",  "--libtokyocabinet-file=tmp/file", "--libtokyocabinet-optimize", "--check-args", 0 };

  test_success(exec_cmdline(GEARMAND_BINARY, args));
  return TEST_SUCCESS;
}

static test_return_t collection_init(void *object)
{
  test_skip(TEST_SUCCESS, test_for_HAVE_LIBTOKYOCABINET(object));
  const char *argv[3]= { "test_gearmand", "--libtokyocabinet-file=tests/gearman.tcb", "--queue-type=libtokyocabinet" };

  unlink("tests/gearman.tcb");

  Context *test= (Context *)object;
  assert(test);

  test_truth(test->initialize(3, argv));

  return TEST_SUCCESS;
}

static test_return_t collection_cleanup(void *object)
{
  Context *test= (Context *)object;
  test->reset();
  unlink("tests/gearman.tcb");

  return TEST_SUCCESS;
}


static void *world_create(server_startup_st& servers, test_return_t& error)
{
  Context *test= new Context(TOKYOCABINET_TEST_PORT, servers);
  if (not test)
  {
    error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }
  unlink("tests/gearman.tcb");

  return test;
}

static bool world_destroy(void *object)
{
  Context *test= (Context *)object;

  unlink("tests/gearman.tcb");
  delete test;

  return TEST_SUCCESS;
}

test_st gearmand_basic_option_tests[] ={
  {"--libtokyocabinet-file=tmp/file --libtokyocabinet-optimize", 0, gearmand_basic_option_test },
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
  {"gearmand options", test_for_HAVE_LIBTOKYOCABINET, 0, gearmand_basic_option_tests},
  {"tokyocabinet queue", collection_init, collection_cleanup, tests},
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  world->collections= collection;
  world->_create= world_create;
  world->_destroy= world_destroy;
}
