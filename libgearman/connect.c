/* Gearman server and library
 * Copyright (C) 2008 Brian Aker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "common.h"

static gearman_return set_hostinfo(gearman_server_st *server)
{
  struct addrinfo *ai;
  struct addrinfo hints;
  int e;
  char str_port[NI_MAXSERV];

  sprintf(str_port, "%u", server->port);

  memset(&hints, 0, sizeof(hints));

  hints.ai_family= AF_INET;
  hints.ai_socktype= SOCK_STREAM;
  hints.ai_protocol= IPPROTO_TCP;

  e= getaddrinfo(server->hostname, str_port, &hints, &ai);
  if (e != 0)
  {
    WATCHPOINT_STRING(server->hostname);
    WATCHPOINT_STRING(gai_strerror(e));
    return GEARMAN_HOST_LOOKUP_FAILURE;
  }

  if (server->address_info)
    freeaddrinfo(server->address_info);
  server->address_info= ai;

  return GEARMAN_SUCCESS;
}

static gearman_return set_socket_options(gearman_server_st *ptr)
{
  if (ptr->root->flags & GEAR_NO_BLOCK)
  {
    int error;
    struct linger linger;
    struct timeval waittime;

    waittime.tv_sec= 10;
    waittime.tv_usec= 0;

    linger.l_onoff= 1; 
    linger.l_linger= GEARMAN_DEFAULT_TIMEOUT; 
    error= setsockopt(ptr->fd, SOL_SOCKET, SO_LINGER, 
                      &linger, (socklen_t)sizeof(struct linger));
    WATCHPOINT_ASSERT(error == 0);

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_SNDTIMEO, 
                      &waittime, (socklen_t)sizeof(struct timeval));
    WATCHPOINT_ASSERT(error == 0);

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_RCVTIMEO, 
                      &waittime, (socklen_t)sizeof(struct timeval));
    WATCHPOINT_ASSERT(error == 0);
  }

  if (ptr->root->flags & GEAR_TCP_NODELAY)
  {
    int flag= 1;
    int error;

    error= setsockopt(ptr->fd, IPPROTO_TCP, TCP_NODELAY, 
                      &flag, (socklen_t)sizeof(int));
    WATCHPOINT_ASSERT(error == 0);
  }

  if (ptr->root->send_size)
  {
    int error;

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_SNDBUF, 
                      &ptr->root->send_size, (socklen_t)sizeof(int));
    WATCHPOINT_ASSERT(error == 0);
  }

  if (ptr->root->recv_size)
  {
    int error;

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_SNDBUF, 
                      &ptr->root->recv_size, (socklen_t)sizeof(int));
    WATCHPOINT_ASSERT(error == 0);
  }

  /* For the moment, not getting a nonblocking mode will not be fatal */
  if (ptr->root->flags & GEAR_NO_BLOCK)
  {
    int flags;

    flags= fcntl(ptr->fd, F_GETFL, 0);
    unlikely (flags != -1)
    {
      (void)fcntl(ptr->fd, F_SETFL, flags | O_NONBLOCK);
    }
  }

  return GEARMAN_SUCCESS;
}

static gearman_return network_connect(gearman_server_st *ptr)
{
  if (ptr->fd == -1)
  {
    struct addrinfo *use;

    /* Old connection junk still is in the structure */
    WATCHPOINT_ASSERT(ptr->cursor_active == 0);

    if (ptr->sockaddr_inited == GEARMAN_NOT_ALLOCATED || 
        (!(ptr->root->flags & GEAR_USE_CACHE_LOOKUPS)))
    {
      gearman_return rc;

      rc= set_hostinfo(ptr);
      if (rc != GEARMAN_SUCCESS)
        return rc;
      ptr->sockaddr_inited= GEARMAN_ALLOCATED;
    }

    use= ptr->address_info;
    /* Create the socket */
    while (use != NULL)
    {
      if ((ptr->fd= socket(use->ai_family, 
                           use->ai_socktype, 
                           use->ai_protocol)) < 0)
      {
        ptr->cached_errno= errno;
        WATCHPOINT_ERRNO(errno);
        return GEARMAN_CONNECTION_SOCKET_CREATE_FAILURE;
      }

      (void)set_socket_options(ptr);

      /* connect to server */
test_connect:
      if (connect(ptr->fd, 
                  use->ai_addr, 
                  use->ai_addrlen) < 0)
      {
        switch (errno) {
          /* We are spinning waiting on connect */
        case EALREADY:
        case EINPROGRESS:
          {
            struct pollfd fds[1];
            int error;

            memset(&fds, 0, sizeof(struct pollfd));
            fds[0].fd= ptr->fd;
            fds[0].events= POLLOUT |  POLLERR;
            error= poll(fds, 1, ptr->root->connect_timeout);

            if (error != 1)
            {
              ptr->cached_errno= errno;
              WATCHPOINT_ERRNO(ptr->cached_errno);
              WATCHPOINT_NUMBER(ptr->root->connect_timeout);
              close(ptr->fd);
              ptr->fd= -1;
              return GEARMAN_ERRNO;
            }

            break;
          }
          /* We are spinning waiting on connect */
        case EINTR:
          goto test_connect;
        case EISCONN: /* We were spinning waiting on connect */
          break;
        default:
          ptr->cached_errno= errno;
          WATCHPOINT_ERRNO(ptr->cached_errno);
          close(ptr->fd);
          ptr->fd= -1;
          if (ptr->root->retry_timeout)
          {
            struct timeval next_time;

            gettimeofday(&next_time, NULL);
            ptr->next_retry= next_time.tv_sec + ptr->root->retry_timeout;
          }
        }
      }
      else
      {
        WATCHPOINT_ASSERT(ptr->cursor_active == 0);
        return GEARMAN_SUCCESS;
      }
      use = use->ai_next;
    }
  }

  if (ptr->fd == -1)
    return GEARMAN_ERRNO; /* The last error should be from connect() */

  return GEARMAN_SUCCESS; /* The last error should be from connect() */
}


gearman_return gearman_connect(gearman_server_st *ptr)
{
  gearman_return rc= GEARMAN_NO_SERVERS;

  if (ptr->type == GEARMAN_SERVER_TYPE_INTERNAL)
    return GEARMAN_SUCCESS;

  if (ptr->root->retry_timeout)
  {
    struct timeval next_time;

    gettimeofday(&next_time, NULL);
    if (next_time.tv_sec < ptr->next_retry)
      return GEARMAN_TIMEOUT;
  }

  /* We need to clean up the multi startup piece */
  rc= network_connect(ptr);

  if (rc != GEARMAN_SUCCESS)
    WATCHPOINT_ERROR(rc);

  return rc;
}
