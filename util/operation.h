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

#include <cstring>
#include <iostream>
#include <vector>

namespace gearman_util {

class Operation {
  typedef std::vector<char> Packet;

public:
  typedef std::vector<Operation> vector;

  Operation(const char *command, size_t command_length, bool expect_response= false) :
    _expect_response(expect_response)
  {
    packet.resize(command_length);
    memcpy(&packet[0], command, command_length);
  }

  size_t size() const
  {
    return packet.size();
  }

  const char* ptr() const
  {
    return &(packet)[0];
  }

  bool has_response() const
  {
    return _expect_response;
  }

  void push(const char *buffer, size_t buffer_size)
  {
    size_t response_size= response.size();
    response.resize(response_size + buffer_size);
    memcpy(&response[0] + response_size, buffer, buffer_size);
  }

  void print() const
  {
    if (response.empty())
      return;

    std::cerr << std::string(&response[0], response.size());
  }

  bool reconnect() const
  {
    return false;
  }

private:
  bool _expect_response;
  Packet packet;
  Packet response;
};

} // namespace gearman_util
