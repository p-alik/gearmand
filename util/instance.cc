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

#include "config.h"

#include "util/instance.h"

#include <cstdio>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>


namespace gearman_util {

void Instance::run()
{
  while (not _operations.empty())
  {
    Operation::vector::reference operation= _operations.back();

    switch (state)
    {
    case NOT_WRITING:
      {
        free_addrinfo();

        struct addrinfo ai;
        memset(&ai, 0, sizeof(struct addrinfo));
        ai.ai_socktype= SOCK_STREAM;
        ai.ai_protocol= IPPROTO_TCP;

        int ret= getaddrinfo(_host.c_str(), _port.c_str(), &ai, &_addrinfo);
        if (ret)
        {
          std::cerr << "Failed to connect on " << _host.c_str() << ":" << _port.c_str() << " with "  << gai_strerror(ret) << std::endl;
          return;
        }
      }
      _addrinfo_next= _addrinfo;
      state= CONNECT;
      break;

    case NEXT_CONNECT_ADDRINFO:
      if (_addrinfo_next->ai_next == NULL)
      {
        std::cerr << "Error connecting to " << _host.c_str() << "." << std::endl;
        return;
      }
      _addrinfo_next= _addrinfo_next->ai_next;


    case CONNECT:
      close_socket();

      _sockfd= socket(_addrinfo_next->ai_family,
                      _addrinfo_next->ai_socktype,
                      _addrinfo_next->ai_protocol);
      if (_sockfd == INVALID_SOCKET)
      {
        perror("socket");
        continue;
      }

      if (connect(_sockfd, _addrinfo_next->ai_addr, _addrinfo_next->ai_addrlen) < 0)
      {
        switch(errno)
        {
        case EAGAIN:
        case EINTR:
          state= CONNECT;
          break;
        case EINPROGRESS:
          state= CONNECTING;
          break;
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:
        default:
          state= NEXT_CONNECT_ADDRINFO;
          break;
        }
      }
      else
      {
        state= CONNECTING;
      }
      break;

    case CONNECTING:
      // Add logic for poll() for nonblocking.
      state= CONNECTED;
      break;

    case CONNECTED:
    case WRITING:
      {
        size_t packet_length= operation.size();
        const char *packet= operation.ptr();

        while(packet_length)
        {
          ssize_t write_size= send(_sockfd, packet, packet_length, 0);

          if (write_size < 0)
          {
            switch(errno)
            {
            default:
              std::cerr << "Failed during send(" << strerror(errno) << ")" << std::endl;
              break;
            }
          }

          packet_length-= static_cast<size_t>(write_size);
          packet+= static_cast<size_t>(write_size);
        }
      }
      state= READING;
      break;

    case READING:
      if (operation.has_response())
      {
        size_t total_read;
        ssize_t read_length;

        do
        {
          char buffer[1024];
          read_length= recv(_sockfd, buffer, sizeof(buffer), 0);

          if (read_length < 0)
          {
            switch(errno)
            {
            default:
              std::cerr << "Error occured while reading data from " << _host.c_str() << std::endl;
              return;
            }
          }

          operation.push(buffer, static_cast<size_t>(read_length));
          total_read+= static_cast<size_t>(read_length);
        } while (more_to_read());
      } // end has_response

      state= FINISHED;
      break;

    case FINISHED:
      operation.print();

      if (operation.reconnect())
      {
      }

      _operations.pop_back();
      state= CONNECTED;
      break;
    } // end switch
  }
} // end run()

bool Instance::more_to_read() const
{
  struct pollfd fds;
  fds.fd= _sockfd;
  fds.events = POLLIN;

  if (poll(&fds, 1, 5) < 1) // Default timeout is 5
  {
    return false;
  }

  return true;
}

void Instance::close_socket()
{
  if (_sockfd == INVALID_SOCKET)
    return;

  /* in case of death shutdown to avoid blocking at close() */
  if (shutdown(_sockfd, SHUT_RDWR) == SOCKET_ERROR && get_socket_errno() != ENOTCONN)
  {
    perror("shutdown");
  }
  else if (closesocket(_sockfd) == SOCKET_ERROR)
  {
    perror("close");
  }

  _sockfd= INVALID_SOCKET;
}

void Instance::free_addrinfo()
{
  if (not _addrinfo)
    return;

  freeaddrinfo(_addrinfo);
  _addrinfo= NULL;
  _addrinfo_next= NULL;
}

} // namespace gearman_util
