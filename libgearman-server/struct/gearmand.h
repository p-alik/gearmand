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

#include "libgearman-server/struct/server.h"

#include "libgearman-server/struct/port.h"

#include <vector>
#include <cstring>

struct gearmand_st
{
  gearmand_verbose_t verbose;
  gearmand_error_t ret;
  int backlog; // Set socket backlog for listening connection
  bool is_listen_event;
  bool is_wakeup_event;
  bool _exceptions;
  int timeout;
  uint32_t threads;
  uint32_t thread_count;
  uint32_t free_dcon_count;
  uint32_t max_thread_free_dcon_count;
  int wakeup_fd[2];
  char *host;
  gearmand_log_fn *log_fn;
  void *log_context;
  struct event_base *base;
  gearmand_thread_st *thread_list;
  gearmand_thread_st *thread_add_next;
  gearmand_con_st *free_dcon_list;
  gearman_server_st server;
  struct event wakeup_event;
  std::vector<gearmand_port_st> _port_list;

  gearmand_st(const char *host_,
              uint32_t threads_,
              int backlog_,
              const gearmand_verbose_t verbose_,
              bool exceptions_) :
    verbose(verbose_),
    ret(GEARMAN_SUCCESS),
    backlog(backlog_),
    is_listen_event(false),
    is_wakeup_event(false),
    _exceptions(exceptions_),
    timeout(-1),
    threads(threads_),
    thread_count(0),
    free_dcon_count(0),
    max_thread_free_dcon_count(0),
    host(NULL),
    log_fn(NULL),
    log_context(NULL),
    base(NULL),
    thread_list(NULL),
    thread_add_next(NULL),
    free_dcon_list(NULL)
  {
    if (host)
    {
      host= strdup(host_);
    }
    wakeup_fd[0]= -1;
    wakeup_fd[1]= -1;
  }

  ~gearmand_st()
  {
    if (host)
    {
      free(host);
    }
  }
  
  bool exceptions() const
  {
    return _exceptions;
  }
};
