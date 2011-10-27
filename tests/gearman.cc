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


#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#include <tests/ports.h>
#include <tests/workers.h>
#include <tests/start_worker.h>

static std::string executable;

#define WORKER_FUNCTION_NAME "echo_function"

static test_return_t help_test(void *)
{
  const char *args[]= { "-H", 0 };

  test_compare(true, exec_cmdline(executable, args));
  return TEST_SUCCESS;
}

static test_return_t unknown_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "-p %d", int(default_port()));
  const char *args[]= { buffer, "--unknown", 0 };

  // The argument doesn't exist, so we should see an error
  test_false(exec_cmdline(executable, args));

  return TEST_SUCCESS;
}

static test_return_t client_background_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "-p %d", int(default_port()));
  const char *args[]= { buffer, "-f", WORKER_FUNCTION_NAME, "-b", "payload", 0 };

  // The argument doesn't exist, so we should see an error
  test_true(exec_cmdline(executable, args));

  return TEST_SUCCESS;
}

#define REGRESSION_FUNCTION_833394 "55_char_function_name_________________________________"

static test_return_t regression_833394_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "-p %d", int(default_port()));
  const char *args[]= { buffer, "-f", REGRESSION_FUNCTION_833394, "-b", "payload", 0 };

  gearman_function_t echo_react_fn_v2= gearman_function_create(echo_or_react_worker_v2);
  worker_handle_st *worker= test_worker_start(CLIENT_TEST_PORT, NULL, REGRESSION_FUNCTION_833394, echo_react_fn_v2, NULL, gearman_worker_options_t());

  // The argument doesn't exist, so we should see an error
  test_true(exec_cmdline(executable, args));

  delete worker;

  return TEST_SUCCESS;
}

test_st gearman_tests[] ={
  { "--help", 0, help_test },
  { "-H", 0, help_test },
  { "--unknown", 0, unknown_test },
  { "-f echo -b payload", 0, client_background_test },
  { "lp:833394", 0, regression_833394_test },
  { 0, 0, 0 }
};

collection_st collection[] ={
  {"gearman", 0, 0, gearman_tests},
  {0, 0, 0, 0}
};

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  const char *argv[1]= { "gearman_gearmand" };
  if (server_startup(servers, "gearmand", GEARADMIN_TEST_PORT, 1, argv) == false)
  {
    error= TEST_FAILURE;
  }
  
  // Echo function
  gearman_function_t echo_react_fn_v2= gearman_function_create(echo_or_react_worker_v2);
  worker_handle_st *worker= test_worker_start(CLIENT_TEST_PORT, NULL, WORKER_FUNCTION_NAME, echo_react_fn_v2, NULL, gearman_worker_options_t());

  return worker;
}

static bool world_destroy(void *object)
{
  worker_handle_st *worker= (worker_handle_st *)object;
  delete worker;

  return TEST_SUCCESS;
}


void get_world(Framework *world)
{
  executable= "./bin/gearman";
  world->collections= collection;
  world->_create= world_create;
  world->_destroy= world_destroy;
}
