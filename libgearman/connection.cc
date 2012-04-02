/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2008 Brian Aker, Eric Day
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

/**
 * @file
 * @brief Connection Definitions
 */

#include <config.h>
#include <libgearman/common.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <unistd.h>

#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>    /* for TCP_NODELAY */
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

static gearman_return_t gearman_connection_set_option(gearman_connection_st *connection,
                                                      gearman_connection_options_t options,
                                                      bool value);



gearman_return_t gearman_connection_st::connect_poll()
{
  struct pollfd fds[1];
  fds[0].fd= fd;
  fds[0].events= POLLOUT;

  size_t loop_max= 5;

#if 0
  if (universal.timeout == 0)
  {
    return gearman_error(universal, GEARMAN_TIMEOUT, "not connected");
  }
#endif

  while (--loop_max) // Should only loop on cases of ERESTART or EINTR
  {
    int error= poll(fds, 1, GEARMAN_DEFAULT_CONNECT_TIMEOUT);
    switch (error)
    {
    case 1:
      {
        int err;
        socklen_t len= sizeof (err);
        // We replace errno with err if getsockopt() passes, but err has been
        // set.
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0)
        {
          // We check the value to see what happened wth the socket.
          if (err == 0)
          {
            return GEARMAN_SUCCESS;
          }
          errno= err;
        }

        return gearman_perror(universal, "getsockopt() failed");
      }

    case 0:
      {
        return gearman_error(universal, GEARMAN_TIMEOUT, "timeout occurred while trying to connect");
      }

    default: // A real error occurred and we need to completely bail
      switch (get_socket_errno())
      {
#ifdef TARGET_OS_LINUX
      case ERESTART:
#endif
      case EINTR:
        continue;

      case EFAULT:
      case ENOMEM:
        return gearman_perror(universal, "poll() failure");

      case EINVAL:
        return gearman_perror(universal, "RLIMIT_NOFILE exceeded, or if OSX the timeout value was invalid");

      default: // This should not happen
        if (fds[0].revents & POLLERR)
        {
          int err;
          socklen_t len= sizeof (err);
          (void)getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
          errno= err;
        }
        else
        {
          errno= get_socket_errno();
        }

        assert_msg(fd != INVALID_SOCKET, "poll() was passed an invalid file descriptor");

        return gearman_perror(universal, "socket error occurred");
      }
    }
  }

  // This should only be possible from ERESTART or EINTR;
  return gearman_perror(universal, "connection failed (error should be from either ERESTART or EINTR");
}

/**
 * @addtogroup gearman_connection_static Static Connection Declarations
 * @ingroup gearman_connection
 * @{
 */

gearman_connection_st::gearman_connection_st(gearman_universal_st &universal_arg,
                                             gearman_connection_options_t *options_args) :
  state(GEARMAN_CON_UNIVERSAL_ADDRINFO),
  send_state(GEARMAN_CON_SEND_STATE_NONE),
  recv_state(GEARMAN_CON_RECV_UNIVERSAL_NONE),
  port(0),
  events(0),
  revents(0),
  fd(-1),
  cached_errno(0),
  created_id(0),
  created_id_next(0),
  send_buffer_size(0),
  send_data_size(0),
  send_data_offset(0),
  recv_buffer_size(0),
  recv_data_size(0),
  recv_data_offset(0),
  universal(universal_arg),
  _recv_packet(NULL)
{
  options.ready= false;
  options.packet_in_use= false;

  if (options_args)
  {
    while (*options_args != GEARMAN_CON_MAX)
    {
      gearman_connection_set_option(this, *options_args, true);
      options_args++;
    }
  }

  if (universal.con_list)
  {
    universal.con_list->prev= this;
  }
  next= universal.con_list;
  prev= NULL;
  universal.con_list= this;
  universal.con_count++;

  context= NULL;
  addrinfo= NULL;
  addrinfo_next= NULL;
  send_buffer_ptr= send_buffer;
  recv_buffer_ptr= recv_buffer;
  host[0]= 0;
}

gearman_connection_st *gearman_connection_create(gearman_universal_st &universal,
                                                 gearman_connection_options_t *options)
{
  gearman_connection_st *connection= new (std::nothrow) gearman_connection_st(universal, options);
  if (connection == NULL)
  {
    gearman_perror(universal, "gearman_connection_st new");
    return NULL;
  }

  return connection;
}

gearman_connection_st *gearman_connection_create_args(gearman_universal_st& universal,
                                                      const char *host, in_port_t port)
{
  gearman_connection_st *connection= gearman_connection_create(universal, NULL);
  if (connection == NULL)
  {
    return NULL;
  }

  connection->set_host(host, port);

  if (gearman_failed(connection->lookup()))
  {
    gearman_gerror(universal, GEARMAN_GETADDRINFO);
    delete connection;
    return NULL;
  }

  return connection;
}

gearman_connection_st *gearman_connection_copy(gearman_universal_st& universal,
                                               const gearman_connection_st& from)
{
  gearman_connection_st *connection= gearman_connection_create(universal, NULL);

  if (connection == NULL)
  {
    return NULL;
  }

  connection->options.ready= from.options.ready;
  // @todo Is this right?
  connection->options.packet_in_use= from.options.packet_in_use;

  strcpy(connection->host, from.host);
  connection->port= from.port;

  return connection;
}

gearman_connection_st::~gearman_connection_st()
{
  if (fd != INVALID_SOCKET)
  {
    close_socket();
  }

  reset_addrinfo();

  { // Remove from universal list
    if (universal.con_list == this)
    {
      universal.con_list= next;
    }

    if (prev)
    {
      prev->next= next;
    }

    if (next)
    {
      next->prev= prev;
    }

    universal.con_count--;
  }

  free_private_packet();
}

void gearman_connection_st::free_private_packet()
{
  if (options.packet_in_use)
  {
    gearman_packet_free(&_packet);
    options.packet_in_use= false;
  }
}

gearman_return_t gearman_connection_set_option(gearman_connection_st *connection,
                                               gearman_connection_options_t options,
                                               bool value)
{
  switch (options)
  {
  case GEARMAN_CON_READY:
    connection->options.ready= value;
    break;

  case GEARMAN_CON_PACKET_IN_USE:
    connection->options.packet_in_use= value;
    break;

  case GEARMAN_CON_IGNORE_LOST_CONNECTION:
    break;

  case GEARMAN_CON_CLOSE_AFTER_FLUSH:
    break;

  case GEARMAN_CON_EXTERNAL_FD:
  case GEARMAN_CON_MAX:
  default:
    return GEARMAN_INVALID_COMMAND;
  }

  return GEARMAN_SUCCESS;
}

/**
 * Set socket options for a connection.
 */
static gearman_return_t _con_setsockopt(gearman_connection_st *connection);

/** @} */

/*
 * Public Definitions
 */

void gearman_connection_st::set_host(const char *host_arg, const in_port_t port_arg)
{
  reset_addrinfo();

  strncpy(host, host_arg == NULL ? GEARMAN_DEFAULT_TCP_HOST : host_arg, GEARMAN_NI_MAXHOST);
  host[GEARMAN_NI_MAXHOST - 1]= 0;

  port= in_port_t(port_arg == 0 ? GEARMAN_DEFAULT_TCP_PORT : port_arg);
}

void gearman_connection_st::close_socket()
{
  if (fd == INVALID_SOCKET)
  {
    return;
  }

  /* in case of death shutdown to avoid blocking at close_socket() */
  if (shutdown(fd, SHUT_RDWR) == SOCKET_ERROR && get_socket_errno() != ENOTCONN)
  {
#if 0
    gearman_perror(universal, "shutdown");
    assert(errno != ENOTSOCK);
#endif
  }
  else
  {
    if (closesocket(fd) == SOCKET_ERROR)
    {
#if 0
      gearman_perror(universal, "close");
#endif
    }
  }

  state= GEARMAN_CON_UNIVERSAL_ADDRINFO;
  fd= INVALID_SOCKET;
  events= 0;
  revents= 0;

  send_state= GEARMAN_CON_SEND_STATE_NONE;
  send_buffer_ptr= send_buffer;
  send_buffer_size= 0;
  send_data_size= 0;
  send_data_offset= 0;

  recv_state= GEARMAN_CON_RECV_UNIVERSAL_NONE;
  free_recv_packet();

  recv_buffer_ptr= recv_buffer;
  recv_buffer_size= 0;
}

void gearman_connection_st::free_recv_packet()
{
  if (_recv_packet)
  {
    gearman_packet_free(recv_packet());
    _recv_packet= NULL;
  }
}

void gearman_connection_st::reset_addrinfo()
{
  if (addrinfo)
  {
    freeaddrinfo(addrinfo);
    addrinfo= NULL;
  }

  addrinfo_next= NULL;
}

gearman_return_t gearman_connection_st::send_packet(const gearman_packet_st& packet_arg, const bool flush_buffer)
{
  switch (send_state)
  {
  case GEARMAN_CON_SEND_STATE_NONE:
    if (packet_arg.options.complete == false)
    {
      return gearman_error(universal, GEARMAN_INVALID_PACKET, "packet not complete");
    }

    /* Pack first part of packet, which is everything but the payload. */
    while (1)
    {
      gearman_return_t rc;
      { // Scoping to shut compiler up about switch/case jump
        size_t send_size= gearman_packet_pack(packet_arg,
                                              send_buffer +send_buffer_size,
                                              GEARMAN_SEND_BUFFER_SIZE -send_buffer_size, rc);

        if (gearman_success(rc))
        {
          send_buffer_size+= send_size;
          break;
        }
        else if (rc == GEARMAN_IGNORE_PACKET)
        {
          return GEARMAN_SUCCESS;
        }
        else if (rc != GEARMAN_FLUSH_DATA)
        {
          return rc;
        }
      }

      /* We were asked to flush when the buffer is already flushed! */
      if (send_buffer_size == 0)
      {
        return gearman_universal_set_error(universal, GEARMAN_SEND_BUFFER_TOO_SMALL, GEARMAN_AT,
                                           "send buffer too small (%u)", GEARMAN_SEND_BUFFER_SIZE);
      }

      /* Flush buffer now if first part of packet won't fit in. */
      send_state= GEARMAN_CON_SEND_UNIVERSAL_PRE_FLUSH;

    case GEARMAN_CON_SEND_UNIVERSAL_PRE_FLUSH:
      {
        gearman_return_t ret= flush();
        if (gearman_failed(ret))
        {
          return ret;
        }
      }
    }

    /* Return here if we have no data to send. */
    if (packet_arg.data_size == 0)
    {
      break;
    }

    /* If there is any room in the buffer, copy in data. */
    if (packet_arg.data and (GEARMAN_SEND_BUFFER_SIZE - send_buffer_size) > 0)
    {
      send_data_offset= GEARMAN_SEND_BUFFER_SIZE - send_buffer_size;
      if (send_data_offset > packet_arg.data_size)
      {
        send_data_offset= packet_arg.data_size;
      }

      memcpy(send_buffer + send_buffer_size, packet_arg.data, send_data_offset);
      send_buffer_size+= send_data_offset;

      /* Return if all data fit in the send buffer. */
      if (send_data_offset == packet_arg.data_size)
      {
        send_data_offset= 0;
        break;
      }
    }

    /* Flush buffer now so we can start writing directly from data buffer. */
    send_state= GEARMAN_CON_SEND_UNIVERSAL_FORCE_FLUSH;

  case GEARMAN_CON_SEND_UNIVERSAL_FORCE_FLUSH:
    {
      gearman_return_t ret= flush();
      if (gearman_failed(ret))
      {
        return ret;
      }
    }

    send_data_size= packet_arg.data_size;

    /* If this is NULL, then gearman_connection_send_data function will be used. */
    if (packet_arg.data == NULL)
    {
      send_state= GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA;
      return GEARMAN_SUCCESS;
    }

    /* Copy into the buffer if it fits, otherwise flush from packet buffer. */
    send_buffer_size= packet_arg.data_size - send_data_offset;
    if (send_buffer_size < GEARMAN_SEND_BUFFER_SIZE)
    {
      memcpy(send_buffer,
             static_cast<char *>(const_cast<void *>(packet_arg.data)) + send_data_offset,
             send_buffer_size);
      send_data_size= 0;
      send_data_offset= 0;
      break;
    }

    send_buffer_ptr= static_cast<char *>(const_cast<void *>(packet_arg.data)) + send_data_offset;
    send_state= GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA;

  case GEARMAN_CON_SEND_UNIVERSAL_FLUSH:
  case GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA:
    return flush();
  }

  if (flush_buffer)
  {
    send_state= GEARMAN_CON_SEND_UNIVERSAL_FLUSH;
    return flush();
  }

  send_state= GEARMAN_CON_SEND_STATE_NONE;

  return GEARMAN_SUCCESS;
}

size_t gearman_connection_st::send_and_flush(const void *data, size_t data_size, gearman_return_t *ret_ptr)
{
  if (send_state != GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA)
  {
    return gearman_error(universal, GEARMAN_NOT_FLUSHING, "not flushing");
  }

  if (data_size > (send_data_size - send_data_offset))
  {
    return gearman_error(universal, GEARMAN_DATA_TOO_LARGE, "data too large");
  }

  send_buffer_ptr= static_cast<char *>(const_cast<void *>(data));
  send_buffer_size= data_size;

  *ret_ptr= flush();

  return data_size -send_buffer_size;
}

gearman_return_t gearman_connection_st::lookup()
{
  if (addrinfo)
  {
    freeaddrinfo(addrinfo);
    addrinfo= NULL;
  }

  char port_str[GEARMAN_NI_MAXSERV];
  snprintf(port_str, GEARMAN_NI_MAXSERV, "%hu", uint16_t(port));

  struct addrinfo ai;
  memset(&ai, 0, sizeof(struct addrinfo));
  ai.ai_socktype= SOCK_STREAM;
  ai.ai_protocol= IPPROTO_TCP;

  int ret;
  if ((ret= getaddrinfo(host, port_str, &ai, &(addrinfo))))
  {
    return gearman_universal_set_error(universal, GEARMAN_GETADDRINFO, GEARMAN_AT, "getaddrinfo:%s", gai_strerror(ret));
  }

  addrinfo_next= addrinfo;
  state= GEARMAN_CON_UNIVERSAL_CONNECT;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_connection_st::flush()
{
  while (1)
  {
    switch (state)
    {
    case GEARMAN_CON_UNIVERSAL_ADDRINFO:
      {
        gearman_return_t ret= lookup();

        if (gearman_failed(ret))
        {
          return ret;
        }
      }

    case GEARMAN_CON_UNIVERSAL_CONNECT:
      if (fd != INVALID_SOCKET)
      {
        close_socket();
      }

      if (addrinfo_next == NULL)
      {
        state= GEARMAN_CON_UNIVERSAL_ADDRINFO;
        return gearman_universal_set_error(universal, GEARMAN_COULD_NOT_CONNECT, GEARMAN_AT, "%s:%hu", host, uint16_t(port));
      }

      fd= socket(addrinfo_next->ai_family, addrinfo_next->ai_socktype, addrinfo_next->ai_protocol);
      if (fd == INVALID_SOCKET)
      {
        state= GEARMAN_CON_UNIVERSAL_ADDRINFO;
        return gearman_perror(universal, "socket");
      }

      {
        gearman_return_t gret= _con_setsockopt(this);
        if (gearman_failed(gret))
        {
          close_socket();
          return gret;
        }
      }

      while (1)
      {
        if (connect(fd, addrinfo_next->ai_addr, addrinfo_next->ai_addrlen) == 0)
        {
          state= GEARMAN_CON_UNIVERSAL_CONNECTED;
          addrinfo_next= NULL;
          break;
        }

        if (errno == EAGAIN || errno == EINTR)
        {
          continue;
        }

        if (errno == EINPROGRESS)
        {
          gearman_return_t gret= connect_poll();
          if (gearman_failed(gret))
          {
            assert_msg(universal.error.rc != GEARMAN_SUCCESS, "Programmer error, connect_poll() returned an error, but it was not set");
            close_socket();
            return gret;
          }

          state= GEARMAN_CON_UNIVERSAL_CONNECTING;
          break;
        }

        if (errno == ECONNREFUSED || errno == ENETUNREACH || errno == ETIMEDOUT)
        {
          state= GEARMAN_CON_UNIVERSAL_CONNECT;
          addrinfo_next= addrinfo_next->ai_next;
          break;
        }

        gearman_perror(universal, "connect");
        close_socket();
        return GEARMAN_COULD_NOT_CONNECT;
      }

      if (state != GEARMAN_CON_UNIVERSAL_CONNECTING)
      {
        break;
      }

    case GEARMAN_CON_UNIVERSAL_CONNECTING:
      while (1)
      {
        if (revents & (POLLERR | POLLHUP | POLLNVAL))
        {
          state= GEARMAN_CON_UNIVERSAL_CONNECT;
          addrinfo_next= addrinfo_next->ai_next;
          break;
        }
        else if (revents & POLLOUT)
        {
          state= GEARMAN_CON_UNIVERSAL_CONNECTED;
          break;
        }

        set_events(POLLOUT);

        if (gearman_universal_is_non_blocking(universal))
        {
          state= GEARMAN_CON_UNIVERSAL_CONNECTING;
          return gearman_gerror(universal, GEARMAN_IO_WAIT);
        }

        gearman_return_t gret= gearman_wait(universal);
        if (gearman_failed(gret))
        {
          return gret;
        }
      }

      if (state != GEARMAN_CON_UNIVERSAL_CONNECTED)
      {
        break;
      }

    case GEARMAN_CON_UNIVERSAL_CONNECTED:
      while (send_buffer_size != 0)
      {
        ssize_t write_size= ::send(fd, send_buffer_ptr, send_buffer_size, 
                                   gearman_universal_is_non_blocking(universal) ? MSG_NOSIGNAL| MSG_DONTWAIT : MSG_NOSIGNAL);

        if (write_size == 0) // Zero value on send()
        { }
        else if (write_size == -1)
        {
          if (errno == EAGAIN)
          {
            set_events(POLLOUT);

            if (gearman_universal_is_non_blocking(universal))
            {
              gearman_gerror(universal, GEARMAN_IO_WAIT);
              return GEARMAN_IO_WAIT;
            }

            gearman_return_t gret= gearman_wait(universal);
            if (gearman_failed(gret))
            {
              return gret;
            }

            continue;
          }
          else if (errno == EINTR)
          {
            continue;
          }
          else if (errno == EPIPE || errno == ECONNRESET || errno == EHOSTDOWN)
          {
            gearman_return_t ret= gearman_perror(universal, "lost connection to server during send");
            close_socket();
            return ret;
          }

          gearman_return_t ret= gearman_perror(universal, "send");
          close_socket();

          return ret;
        }

        send_buffer_size-= size_t(write_size);
        if (send_state == GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA)
        {
          send_data_offset+= size_t(write_size);
          if (send_data_offset == send_data_size)
          {
            send_data_size= 0;
            send_data_offset= 0;
            break;
          }

          if (send_buffer_size == 0)
          {
            return GEARMAN_SUCCESS;
          }
        }
        else if (send_buffer_size == 0)
        {
          break;
        }

        send_buffer_ptr+= write_size;
      }

      send_state= GEARMAN_CON_SEND_STATE_NONE;
      send_buffer_ptr= send_buffer;

      return GEARMAN_SUCCESS;
    }
  }
}

gearman_packet_st *gearman_connection_st::receiving(gearman_packet_st& packet_arg,
                                                    gearman_return_t& ret,
                                                    const bool recv_data)
{
  switch (recv_state)
  {
  case GEARMAN_CON_RECV_UNIVERSAL_NONE:
    if (state != GEARMAN_CON_UNIVERSAL_CONNECTED)
    {
      gearman_error(universal, GEARMAN_NOT_CONNECTED, "not connected");
      ret= GEARMAN_NOT_CONNECTED;
      return NULL;
    }

    _recv_packet= gearman_packet_create(universal, &packet_arg);
    if (_recv_packet == NULL)
    {
      ret= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      return NULL;
    }

    recv_state= GEARMAN_CON_RECV_UNIVERSAL_READ;

  case GEARMAN_CON_RECV_UNIVERSAL_READ:
    while (1)
    {
      if (recv_buffer_size > 0)
      {
        size_t recv_size= gearman_packet_unpack(*(recv_packet()),
                                                recv_buffer_ptr,
                                                recv_buffer_size, ret);
        recv_buffer_ptr+= recv_size;
        recv_buffer_size-= recv_size;

        if (gearman_success(ret))
        {
          break;
        }
        else if (ret != GEARMAN_IO_WAIT)
        {
          close_socket();
          return NULL;
        }
      }

      /* Shift buffer contents if needed. */
      if (recv_buffer_size > 0)
      {
        memmove(recv_buffer, recv_buffer_ptr, recv_buffer_size);
      }
      recv_buffer_ptr= recv_buffer;

      size_t recv_size= recv_socket(recv_buffer + recv_buffer_size, GEARMAN_RECV_BUFFER_SIZE - recv_buffer_size, ret);
      if (gearman_failed(ret))
      {
        return NULL;
      }

      recv_buffer_size+= recv_size;
    }

    if (packet_arg.data_size == 0)
    {
      recv_state= GEARMAN_CON_RECV_UNIVERSAL_NONE;
      break;
    }

    recv_data_size= packet_arg.data_size;

    if (not recv_data)
    {
      recv_state= GEARMAN_CON_RECV_STATE_READ_DATA;
      break;
    }

    assert(packet_arg.universal);
    packet_arg.data= gearman_malloc((*packet_arg.universal), packet_arg.data_size);
    if (packet_arg.data == NULL)
    {
      ret= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      close_socket();
      return NULL;
    }

    packet_arg.options.free_data= true;
    recv_state= GEARMAN_CON_RECV_STATE_READ_DATA;

  case GEARMAN_CON_RECV_STATE_READ_DATA:
    while (recv_data_size)
    {
      (void)receiving(static_cast<uint8_t *>(const_cast<void *>(packet_arg.data)) +
                      recv_data_offset,
                      packet_arg.data_size -recv_data_offset, ret);
      if (gearman_failed(ret))
      {
        return NULL;
      }
    }

    recv_state= GEARMAN_CON_RECV_UNIVERSAL_NONE;
    break;
  }

  gearman_packet_st *tmp_packet_arg= recv_packet();
  set_recv_packet(NULL);

  return tmp_packet_arg;
}

size_t gearman_connection_st::receiving(void *data, size_t data_size, gearman_return_t& ret)
{
  size_t recv_size= 0;

  if (recv_data_size == 0)
  {
    ret= GEARMAN_SUCCESS;
    return 0;
  }

  if ((recv_data_size - recv_data_offset) < data_size)
    data_size= recv_data_size - recv_data_offset;

  if (recv_buffer_size > 0)
  {
    if (recv_buffer_size < data_size)
      recv_size= recv_buffer_size;
    else
      recv_size= data_size;

    memcpy(data, recv_buffer_ptr, recv_size);
    recv_buffer_ptr+= recv_size;
    recv_buffer_size-= recv_size;
  }

  if (data_size != recv_size)
  {
    recv_size+= recv_socket(static_cast<uint8_t *>(const_cast<void *>(data)) + recv_size, data_size - recv_size, ret);
    recv_data_offset+= recv_size;
  }
  else
  {
    recv_data_offset+= recv_size;
    ret= GEARMAN_SUCCESS;
  }

  if (recv_data_size == recv_data_offset)
  {
    recv_data_size= 0;
    recv_data_offset= 0;
    recv_state= GEARMAN_CON_RECV_UNIVERSAL_NONE;
  }

  return recv_size;
}

size_t gearman_connection_st::recv_socket(void *data, size_t data_size, gearman_return_t& ret)
{
  ssize_t read_size;

  while (1)
  {
    read_size= ::recv(fd, data, data_size, 0);
    if (read_size == 0)
    {
      gearman_error(universal, GEARMAN_LOST_CONNECTION, "lost connection to server (EOF)");
      close_socket();
      ret= GEARMAN_LOST_CONNECTION;

      return 0;
    }
    else if (read_size == -1)
    {
      if (errno == EAGAIN)
      {
        set_events(POLLIN);

        if (gearman_universal_is_non_blocking(universal))
        {
          ret= gearman_gerror(universal, GEARMAN_IO_WAIT);
          return 0;
        }

        ret= gearman_wait(universal);

        if (ret == GEARMAN_SHUTDOWN_GRACEFUL)
        {
          ret= gearman_kill(gearman_universal_id(universal), GEARMAN_INTERRUPT);

          if (gearman_failed(ret))
          {
            ret= GEARMAN_SHUTDOWN;
          }
        }
        else if (ret == GEARMAN_SHUTDOWN)
        {
          close_socket();
          return 0;
        }

        if (gearman_failed(ret))
        {
          return 0;
        }

        continue;
      }
      else if (errno == EINTR)
      {
        continue;
      }
      else if (errno == EPIPE || errno == ECONNRESET || errno == EHOSTDOWN)
      {
        gearman_perror(universal, "lost connection to server during read");
        ret= GEARMAN_LOST_CONNECTION;
      }
      else
      {
        gearman_perror(universal, "read");
        ret= GEARMAN_ERRNO;
      }

      close_socket();
      return 0;
    }

    break;
  }

  ret= GEARMAN_SUCCESS;
  return size_t(read_size);
}

void gearman_connection_st::set_events(short arg)
{
  if ((events | arg) == events)
    return;

  events|= arg;
}

void gearman_connection_st::set_revents(short arg)
{
  if (arg)
    options.ready= true;

  revents= arg;
  events&= short(~arg);
}

/*
 * Static Definitions
 */

static gearman_return_t _con_setsockopt(gearman_connection_st *connection)
{
  int ret;
  struct linger linger;
  struct timeval waittime;

  ret= 1;
  ret= setsockopt(connection->fd, IPPROTO_TCP, TCP_NODELAY, &ret,
                  socklen_t(sizeof(int)));
  if (ret == -1 && errno != EOPNOTSUPP)
  {
    gearman_perror(connection->universal, "setsockopt(TCP_NODELAY)");
    return GEARMAN_ERRNO;
  }

  linger.l_onoff= 1;
  linger.l_linger= GEARMAN_DEFAULT_SOCKET_TIMEOUT;
  ret= setsockopt(connection->fd, SOL_SOCKET, SO_LINGER, &linger,
                  socklen_t(sizeof(struct linger)));
  if (ret == -1)
  {
    gearman_perror(connection->universal, "setsockopt(SO_LINGER)");
    return GEARMAN_ERRNO;
  }

  waittime.tv_sec= GEARMAN_DEFAULT_SOCKET_TIMEOUT;
  waittime.tv_usec= 0;
  ret= setsockopt(connection->fd, SOL_SOCKET, SO_SNDTIMEO, &waittime,
                  socklen_t(sizeof(struct timeval)));
  if (ret == -1 && errno != ENOPROTOOPT)
  {
    gearman_perror(connection->universal, "setsockopt(SO_SNDTIMEO)");
    return GEARMAN_ERRNO;
  }

  {
    int optval= 1;
    ret= setsockopt(connection->fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    if (ret == -1 && errno != ENOPROTOOPT)
    {
      gearman_perror(connection->universal, "setsockopt(SO_RCVTIMEO)");
      return GEARMAN_ERRNO;
    }
  }


  ret= setsockopt(connection->fd, SOL_SOCKET, SO_RCVTIMEO, &waittime,
                  socklen_t(sizeof(struct timeval)));
  if (ret == -1 && errno != ENOPROTOOPT)
  {
    gearman_perror(connection->universal, "setsockopt(SO_RCVTIMEO)");
    return GEARMAN_ERRNO;
  }

  ret= GEARMAN_DEFAULT_SOCKET_SEND_SIZE;
  ret= setsockopt(connection->fd, SOL_SOCKET, SO_SNDBUF, &ret, socklen_t(sizeof(int)));
  if (ret == -1)
  {
    gearman_perror(connection->universal, "setsockopt(SO_SNDBUF)");
    return GEARMAN_ERRNO;
  }

#if defined(__MACH__) && defined(__APPLE__) || defined(__FreeBSD__)
  {
    ret= 1;
    setsockopt(connection->fd, SOL_SOCKET, SO_NOSIGPIPE, static_cast<void *>(&ret), sizeof(int));

    // This is not considered a fatal error 
    if (ret == -1)
    {
      gearman_perror(connection->universal, "setsockopt(SO_NOSIGPIPE)");
    }
  }
#endif

  ret= GEARMAN_DEFAULT_SOCKET_RECV_SIZE;
  ret= setsockopt(connection->fd, SOL_SOCKET, SO_RCVBUF, &ret, socklen_t(sizeof(int)));
  if (ret == -1)
  {
    gearman_perror(connection->universal, "setsockopt(SO_RCVBUF)");
    return GEARMAN_ERRNO;
  }

  ret= fcntl(connection->fd, F_GETFL, 0);
  if (ret == -1)
  {
    gearman_perror(connection->universal, "fcntl(F_GETFL)");
    return GEARMAN_ERRNO;
  }

  ret= fcntl(connection->fd, F_SETFL, ret | O_NONBLOCK);
  if (ret == -1)
  {
    gearman_perror(connection->universal, "fcntl(F_SETFL)");
    return GEARMAN_ERRNO;
  }

  return GEARMAN_SUCCESS;
}
