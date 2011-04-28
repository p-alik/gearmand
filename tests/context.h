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


#pragma once

#include <string>
#include <unistd.h>
#include <libtest/server.h>

class Context
{
public:
  pid_t gearmand_pid;
  gearman_worker_st *worker;
  in_port_t _port;
  std::string _worker_function_name;
  bool run_worker;

  Context(in_port_t port_arg):
    gearmand_pid(-1),
    worker(NULL),
    _port(port_arg),
    _worker_function_name("queue_test"),
    run_worker(false)
  {
  }

  const char *worker_function_name() const
  {
    return _worker_function_name.c_str();
  }

  in_port_t port() const
  {
    return _port;
  }

  bool initialize(int argc, const char *argv[])
  {
    gearmand_pid= test_gearmand_start(_port, argc, argv);
    if (gearmand_pid == -1)
      return false;

    if ((worker= gearman_worker_create(NULL)) == NULL)
      return false;

    if (gearman_worker_add_server(worker, NULL, _port) != GEARMAN_SUCCESS)
      return false;

    return true;
  }

  void reset()
  {
    test_gearmand_stop(gearmand_pid);
    gearman_worker_free(worker);

    unlink("tests/gearman.sql");
    unlink("tests/gearman.sql-journal");

    worker= NULL;
    gearmand_pid= -1;
  }
};
