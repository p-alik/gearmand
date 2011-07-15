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

#include <sys/socket.h>
#include <errno.h>

#include "util/operation.h"
#include <libgearman/protocol.h>

struct addrinfo;

namespace gearman_util {

class Instance
{
  enum connection_state_t {
    NOT_WRITING,
    NEXT_CONNECT_ADDRINFO,
    CONNECT,
    CONNECTING,
    CONNECTED,
    WRITING,
    READING,
    FINISHED
  };

public:
  Instance() :
    _host("localhost"),
    _port(GEARMAN_DEFAULT_TCP_PORT_STRING),
    _sockfd(INVALID_SOCKET),
    state(NOT_WRITING),
    _addrinfo(0),
    _addrinfo_next(0),
    _operations()
  {
  }

  ~Instance()
  {
    close_socket();
    free_addrinfo();
  }

  void set_host(const std::string &host)
  {
    _host= host;
  }

  void set_port(const std::string &port)
  {
    _port= port;
  }

  bool run();

  void push(Operation *next)
  {
    _operations.push_back(next);
  }

private:
  void close_socket();

  void free_addrinfo();

  bool more_to_read() const;

  std::string _host;
  std::string _port;
  int _sockfd;
  connection_state_t state;
  struct addrinfo *_addrinfo;
  struct addrinfo *_addrinfo_next;
  Operation::vector _operations;
};

} // namespace gearman_util
