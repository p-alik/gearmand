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

#define WORKER_FUNCTION "drizzle_queue_test"

#if defined(HAVE_LIBDRIZZLE) && HAVE_LIBDRIZZLE
#include <libdrizzle-1.0/drizzle_client.h>
#endif

static bool ping_drizzled(void)
{
#if defined(HAVE_LIBDRIZZLE) && HAVE_LIBDRIZZLE
  if (HAVE_LIBDRIZZLE)
  {
    drizzle_st *drizzle= drizzle_create(NULL);

    if (drizzle == NULL)
    {
      return false;
    }

    drizzle_con_st *con;

    if ((con= drizzle_con_create(drizzle, NULL)) == NULL)
    {
      drizzle_free(drizzle);
      return false;
    }

    drizzle_con_set_tcp(con, NULL, 0);
    drizzle_con_set_auth(con, "root", 0);
    drizzle_return_t rc;
    drizzle_result_st *result= drizzle_ping(con, NULL, &rc);

    bool success= bool(result);

    drizzle_result_free(result);
    drizzle_con_free(con);
    drizzle_free(drizzle);

    return success;
  }
#endif

  return false;
}

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static test_return_t gearmand_basic_option_test(void *)
{
  const char *args[]= { "--check-args", 
    "--libdrizzle-host=localhost",
    "--libdrizzle-port=90",
    "--libdrizzle-uds=tmp/foo.socket",
    "--libdrizzle-user=root",
    "--libdrizzle-password=test",
    "--libdrizzle-db=gearman",
    "--libdrizzle-table=gearman",
    "--libdrizzle-mysql",
    0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(gearmand_binary(), args, true));
  return TEST_SUCCESS;
}

static test_return_t collection_init(void *object)
{
  Context *test= (Context *)object;
  assert(test);

  const char *argv[]= { "test_gearmand", "--queue-type=libdrizzle", 0 };

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
  if (has_drizzle_support() == false)
  {
    error= TEST_SKIPPED;
    return NULL;
  }

  if (HAVE_LIBDRIZZLE)
  {
    if (ping_drizzled() == false)
    {
      error= TEST_SKIPPED;
      return NULL;
    }
  }

  return new Context(default_port(), servers);
}

static bool world_destroy(void *object)
{
  Context *test= (Context *)object;

  delete test;

  return TEST_SUCCESS;
}

test_st gearmand_basic_option_tests[] ={
  {"all options", 0, gearmand_basic_option_test },
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
  {"drizzle queue", collection_init, collection_cleanup, tests},
  {"regressions", collection_init, collection_cleanup, regressions},
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  world->collections= collection;
  world->_create= world_create;
  world->_destroy= world_destroy;
}
