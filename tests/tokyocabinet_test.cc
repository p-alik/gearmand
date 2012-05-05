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

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static test_return_t gearmand_basic_option_test(void *)
{
  const char *args[]= { "--check-args",
    "--queue-type=libtokyocabinet",
    "--libtokyocabinet-file=var/tmp/gearman_basic.tcb",
    "--libtokyocabinet-optimize", 
    0 };

  unlink("var/tmp/gearman.tcb");
  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args, true));

  return TEST_SUCCESS;
}

static test_return_t collection_init(void *object)
{
  const char *argv[]= {
    "--libtokyocabinet-file=var/tmp/gearman.tcb",
    "--queue-type=libtokyocabinet",
    0 };

  unlink("var/tmp/gearman.tcb");

  Context *test= (Context *)object;
  assert(test);

  test_truth(test->initialize(2, argv));

  return TEST_SUCCESS;
}

static test_return_t collection_cleanup(void *object)
{
  Context *test= (Context *)object;
  test->reset();
  unlink("var/tmp/gearman.tcb");

  return TEST_SUCCESS;
}


static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (HAVE_LIBTOKYOCABINET == 0)
  {
    error= TEST_SKIPPED;
    return NULL;
  }

  unlink("var/tmp/gearman.tcb");
  return new Context(libtest::default_port(), servers);
}

static bool world_destroy(void *object)
{
  Context *test= (Context *)object;

  unlink("var/tmp/gearman.tcb");
  delete test;

  return TEST_SUCCESS;
}

test_st gearmand_basic_option_tests[] ={
  {"--libtokyocabinet-file=var/tmp/gearman_basic.tcb --libtokyocabinet-optimize", 0, gearmand_basic_option_test },
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
  {"tokyocabinet queue", collection_init, collection_cleanup, tests},
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  world->collections(collection);
  world->create(world_create);
  world->destroy(world_destroy);
}
