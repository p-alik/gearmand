/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Cycle the Gearmand server
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


/*
  Test that we are cycling the servers we are creating during testing.
*/

#include <config.h>
#include <libtest/test.hpp>

using namespace libtest;
#include <libgearman/gearman.h>

#include "tests/start_worker.h"

static gearman_return_t success_fn(gearman_job_st*, void* /* context */)
{
  return GEARMAN_SUCCESS;
}

static test_return_t single_cycle(void *)
{
  gearman_function_t success_function= gearman_function_create(success_fn);
  worker_handle_st *worker= test_worker_start(libtest::default_port(), NULL, "success", success_function, NULL, gearman_worker_options_t());
  test_true(worker);
  test_true(worker->shutdown());
  delete worker;

  return TEST_SUCCESS;
}

static test_return_t kill_test(void *)
{
  libtest::dream(2, 0);

  return TEST_SUCCESS;
}

static test_return_t server_startup_single_TEST(void *obj)
{
  server_startup_st *servers= (server_startup_st*)obj;
  test_compare(true, server_startup(*servers, "gearmand", libtest::get_free_port(), 0, NULL, false));
  test_compare(true, servers->shutdown());


  return TEST_SUCCESS;
}

static test_return_t server_startup_multiple_TEST(void *obj)
{
  server_startup_st *servers= (server_startup_st*)obj;
  for (size_t x= 0; x < 10; x++)
  {
    test_compare(true, server_startup(*servers, "gearmand", libtest::get_free_port(), 0, NULL, false));
  }
  test_compare(true, servers->shutdown());

  return TEST_SUCCESS;
}

static test_return_t shutdown_and_remove_TEST(void *obj)
{
  server_startup_st *servers= (server_startup_st*)obj;
  servers->clear();

  return TEST_SUCCESS;
}

test_st server_startup_TESTS[] ={
  {"server_startup(1)", false, (test_callback_fn*)server_startup_single_TEST },
  {"server_startup(10)", false, (test_callback_fn*)server_startup_multiple_TEST },
  {"shutdown_and_remove()", false, (test_callback_fn*)shutdown_and_remove_TEST },
  {"server_startup(10)", false, (test_callback_fn*)server_startup_multiple_TEST },
  {0, 0, 0}
};

test_st kill_tests[] ={
  {"kill", true, (test_callback_fn*)kill_test },
  {0, 0, 0}
};

test_st worker_tests[] ={
  {"single startup/shutdown", true, (test_callback_fn*)single_cycle },
  {0, 0, 0}
};

static test_return_t collection_INIT(void *object)
{
  server_startup_st *servers= (server_startup_st*)object;
  test_zero(servers->count());
  test_compare(true, server_startup(*servers, "gearmand", libtest::default_port(), 0, NULL, false));

  return TEST_SUCCESS;
}

static test_return_t validate_sanity_INIT(void *object)
{
  server_startup_st *servers= (server_startup_st*)object;

  test_zero(servers->count());

  return TEST_SUCCESS;
}

static test_return_t collection_FINAL(void *object)
{
  server_startup_st *servers= (server_startup_st*)object;
  servers->clear();

  return TEST_SUCCESS;
}

collection_st collection[] ={
  {"kill", validate_sanity_INIT, collection_FINAL, kill_tests },
  {"worker", collection_INIT, collection_FINAL, worker_tests },
  {"server_startup()", validate_sanity_INIT, collection_FINAL, server_startup_TESTS },
  {0, 0, 0, 0}
};

static void *world_create(server_startup_st& servers, test_return_t& )
{
  return &servers;
}

void get_world(Framework *world)
{
  world->collections= collection;
  world->_create= world_create;
}

