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

// Prototypes
#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static bool test_for_HAVE_LIBSQLITE3(test_return_t &error)
{
  if (HAVE_LIBSQLITE3)
  {
    error= TEST_SUCCESS;
    return true;
  }

  error= TEST_SKIPPED;
  return false;
}

static test_return_t gearmand_basic_option_test(void *)
{
  const char *args[]= { "--check-args",
    "--queue-type=libsqlite3",
    "--libsqlite3-db=var/tmp/gearman.sql",
    "--libsqlite3-table=var/tmp/table", 
    0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args, true));
  return TEST_SUCCESS;
}

static test_return_t gearmand_basic_option_without_table_test(void *)
{
  const char *args[]= { "--check-args",
    "--queue-type=libsqlite3",
    "--libsqlite3-db=var/tmp/gearman.sql",
    0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args, true));
  return TEST_SUCCESS;
}

static test_return_t collection_init(void *object)
{
  const char *argv[]= {
    "--libsqlite3-db=var/tmp/gearman.sql",
    "--queue-type=libsqlite3", 
    0 };

  // Delete whatever might have been sitting around for the sql files
  unlink("var/tmp/gearman.sql");
  unlink("var/tmp/gearman.sql-journal");

  Context *test= (Context *)object;
  assert(test);

  test_truth(test->initialize(2, argv));

  return TEST_SUCCESS;
}

static test_return_t collection_cleanup(void *object)
{
  Context *test= (Context *)object;
  test->reset();

  unlink("tests/gearman.sql");
  unlink("tests/gearman.sql-journal");

  return TEST_SUCCESS;
}


static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (not test_for_HAVE_LIBSQLITE3(error))
  {
    return NULL;
  }

  return  new Context(libtest::default_port(), servers);
}

static bool world_destroy(void *object)
{
  Context *test= (Context *)object;

  delete test;

  return TEST_SUCCESS;
}

test_st gearmand_basic_option_tests[] ={
  {"--libsqlite3-db=var/tmp/schema --libsqlite3-table=var/tmp/table", 0, gearmand_basic_option_test },
  {"--libsqlite3-db=var/tmp/schema", 0, gearmand_basic_option_without_table_test },
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

test_st regressions[] ={
  {"lp:734663", 0, lp_734663 },
  {0, 0, 0}
};

collection_st collection[] ={
  {"gearmand options", 0, 0, gearmand_basic_option_tests},
  {"sqlite queue", collection_init, collection_cleanup, tests},
  {"queue regression", collection_init, collection_cleanup, regressions},
#if 0
  {"sqlite queue change table", collection_init, collection_cleanup, tests},
#endif
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  world->collections(collection);
  world->create(world_create);
  world->destroy(world_destroy);
}
