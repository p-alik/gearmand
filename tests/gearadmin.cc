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

#include <libtest/test.hpp>

using namespace libtest;

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#include "tests/ports.h"

static std::string executable;

static test_return_t help_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--port=%d", int(default_port()));
  const char *args[]= { buffer, "--help", 0 };

  test_success(exec_cmdline(executable, args));
  return TEST_SUCCESS;
}

static test_return_t shutdown_test(void *object)
{
  server_startup_st *servers= (server_startup_st *)object;

  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--port=%d", int(default_port()));
  const char *args[]= { buffer, "--shutdown", 0 };

  test_success(exec_cmdline(executable, args));

  // We have already shut the server down so it should fail
  test_failed(exec_cmdline(executable, args));

  Server *server= servers->pop_server();
  test_true(server);
  test_failed(server->ping());

  // Since we killed the server above, we need to reset it
  server->reset();
  delete server;

  return TEST_SUCCESS;
}

static test_return_t version_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--port=%d", int(default_port()));
  const char *args[]= { buffer, "--server-version", 0 };

  test_success(exec_cmdline(executable, args));
  return TEST_SUCCESS;
}

static test_return_t verbose_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--port=%d", int(default_port()));
  const char *args[]= { buffer, "--server-verbose", 0 };

  test_success(exec_cmdline(executable, args));
  return TEST_SUCCESS;
}

static test_return_t status_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--port=%d", int(default_port()));
  const char *args[]= { buffer, "--status", 0 };

  test_success(exec_cmdline(executable, args));
  return TEST_SUCCESS;
}

static test_return_t workers_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--port=%d", int(default_port()));
  const char *args[]= { buffer, "--workers", 0 };

  test_success(exec_cmdline(executable, args));
  return TEST_SUCCESS;
}

static test_return_t create_drop_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--port=%d", int(default_port()));

  const char *create_args[]= { buffer, "--create-function=test_function", 0 };
  test_success(exec_cmdline(executable, create_args));

  const char *drop_args[]= { buffer, "--drop-function=test_function", 0 };
  test_success(exec_cmdline(executable, drop_args));


  return TEST_SUCCESS;
}

static test_return_t getpid_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--port=%d", int(default_port()));
  const char *args[]= { buffer, "--getpid", 0 };

  test_success(exec_cmdline(executable, args));
  return TEST_SUCCESS;
}

static test_return_t unknown_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--port=%d", int(default_port()));
  const char *args[]= { buffer, "--unknown", 0 };

  // The argument doesn't exist, so we should see an error
  test_failed(exec_cmdline(executable, args));
  return TEST_SUCCESS;
}

test_st gearadmin_tests[] ={
  {"--help", 0, help_test},
  {"--server-version", 0, version_test},
  {"--server-verbose", 0, verbose_test},
  {"--status", 0, status_test},
  {"--getpid", 0, getpid_test},
  {"--workers", 0, workers_test},
  {"--create-function and --drop-function", 0, create_drop_test},
  {"--unknown", 0, unknown_test},
  {"--shutdown", 0, shutdown_test}, // Must be run last since it shuts down the server
  {0, 0, 0}
};

collection_st collection[] ={
  {"gearadmin", 0, 0, gearadmin_tests},
  {0, 0, 0, 0}
};

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  const char *argv[1]= { "gearadmin_gearmand" };
  if (not server_startup(servers, GEARADMIN_TEST_PORT, 1, argv))
  {
    error= TEST_FAILURE;
  }

  return &servers;
}


void get_world(Framework *world)
{
  executable= "./bin/gearadmin";
  world->collections= collection;
  world->_create= world_create;
}

