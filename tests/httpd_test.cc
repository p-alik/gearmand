/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  All rights reserved.
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

static char url[1024]= { 0 };

static test_return_t GET_TEST(void *)
{
  libtest::http::GET get(url);

  test_compare(false, get.execute());

  return TEST_SUCCESS;
}

static test_return_t HEAD_TEST(void *)
{
  libtest::http::HEAD head(url);

  test_compare(false, head.execute());

  return TEST_SUCCESS;
}


static void *world_create(server_startup_st& servers, test_return_t& error)
{
  in_port_t http_port= libtest::get_free_port();
  snprintf(url, sizeof(url), "http://localhost:%d/", int(http_port));

  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--http-port=%d", int(http_port));
  const char *argv[]= { "--protocol=http", buffer, 0 };
  if (server_startup(servers, "gearmand", libtest::default_port(), 2, argv) == false)
  {
    error= TEST_FAILURE;
    return NULL;
  }

  return NULL;
}

static test_return_t check_for_curl(void *)
{
  test_skip(true, HAVE_LIBCURL);
  return TEST_SUCCESS;
}


test_st GET_TESTS[] ={
  { "GET /", 0, GET_TEST },
  { 0, 0, 0 }
};

test_st HEAD_TESTS[] ={
  { "HEAD /", 0, HEAD_TEST },
  { 0, 0, 0 }
};

test_st regression_TESTS[] ={
  { 0, 0, 0 }
};

collection_st collection[] ={
  { "GET", check_for_curl, 0, GET_TESTS },
  { "regression", 0, 0, regression_TESTS },
  { 0, 0, 0, 0 }
};

void get_world(Framework *world)
{
  world->collections= collection;
  world->_create= world_create;
}
