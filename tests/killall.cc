/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Kill all Gearmand servers that we might use during testing (i.e. cleanup previous runs)
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

#include <tests/ports.h>

static test_return_t kill_test(void *)
{
  for (size_t port= GEARMAN_BASE_TEST_PORT; port <= GEARMAN_MAX_TEST_PORT; port++)
  {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "--port=%d", int(port));
    const char *args[]= { buffer, "--shutdown", 0 };

    exec_cmdline("bin/gearadmin", args);
  }

  return TEST_SUCCESS;
}

test_st kill_tests[] ={
  {"via port", true, (test_callback_fn*)kill_test },
  {0, 0, 0}
};

collection_st collection[] ={
  {"killall", 0, 0, kill_tests},
  {0, 0, 0, 0}
};

void get_world(Framework *world)
{
  world->collections= collection;
}


