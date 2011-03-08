/* - mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2011 Data Differential
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
    _addrinfo_next(0)
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

  void run();

  void push(const Operation &next)
  {
    _operations.push_back(next);
  }

private:
  void close_socket();

  void free_addrinfo();

  bool more_to_read() const;

private:
    std::string _host;
    std::string _port;
    int _sockfd;
    connection_state_t state;
    struct addrinfo *_addrinfo;
    struct addrinfo *_addrinfo_next;
    Operation::vector _operations;
};

} // namespace gearman_util
