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

#include <libgearman/common.h>

#include <libgearman/connection.h>
#include <libgearman/packet.hpp>
#include <libgearman/universal.hpp>

#include <assert.h>
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

static void gearman_connection_reset_addrinfo(gearman_connection_st *connection);

static gearman_return_t gearman_connection_set_option(gearman_connection_st *connection,
                                                      gearman_connection_options_t options,
                                                      bool value);




/**
 * @addtogroup gearman_connection_static Static Connection Declarations
 * @ingroup gearman_connection
 * @{
 */

gearman_connection_st *gearman_connection_create(gearman_universal_st &universal,
                                                 gearman_connection_options_t *options)
{
  gearman_connection_st *connection= new (std::nothrow) gearman_connection_st(universal);
  if (not connection)
  {
    gearman_perror(universal, "gearman_connection_st new");
    return NULL;
  }

  connection->options.ready= false;
  connection->options.packet_in_use= false;
  connection->options.ignore_lost_connection= false;
  connection->options.close_after_flush= false;

  if (options)
  {
    while (*options != GEARMAN_CON_MAX)
    {
      gearman_connection_set_option(connection, *options, true);
      options++;
    }
  }

  connection->state= GEARMAN_CON_UNIVERSAL_ADDRINFO;
  connection->send_state= GEARMAN_CON_SEND_STATE_NONE;
  connection->recv_state= GEARMAN_CON_RECV_UNIVERSAL_NONE;
  connection->port= 0;
  connection->events= 0;
  connection->revents= 0;
  connection->fd= -1;
  connection->created_id= 0;
  connection->created_id_next= 0;
  connection->send_buffer_size= 0;
  connection->send_data_size= 0;
  connection->send_data_offset= 0;
  connection->recv_buffer_size= 0;
  connection->recv_data_size= 0;
  connection->recv_data_offset= 0;

  if (universal.con_list != NULL)
    universal.con_list->prev= connection;
  connection->next= universal.con_list;
  connection->prev= NULL;
  universal.con_list= connection;
  universal.con_count++;

  connection->context= NULL;
  connection->addrinfo= NULL;
  connection->addrinfo_next= NULL;
  connection->send_buffer_ptr= connection->send_buffer;
  connection->recv_packet= NULL;
  connection->recv_buffer_ptr= connection->recv_buffer;
  connection->host[0]= 0;

  return connection;
}

gearman_connection_st *gearman_connection_create_args(gearman_universal_st& universal,
                                                      const char *host, in_port_t port)
{
  gearman_connection_st *connection= gearman_connection_create(universal, NULL);
  if (not connection)
    return NULL;

  gearman_connection_set_host(connection, host, port);

  return connection;
}

gearman_connection_st *gearman_connection_copy(gearman_universal_st& universal,
                                               const gearman_connection_st& from)
{
  gearman_connection_st *connection= gearman_connection_create(universal, NULL);

  if (not connection)
    return connection;

  connection->options.ready= from.options.ready;
  connection->options.packet_in_use= from.options.packet_in_use;
  connection->options.ignore_lost_connection= from.options.ignore_lost_connection;
  connection->options.close_after_flush= from.options.close_after_flush;

  strcpy(connection->host, from.host);
  connection->port= from.port;

  return connection;
}

void gearman_connection_free(gearman_connection_st *connection)
{
  if (connection->fd != -1)
    gearman_connection_close(connection);

  gearman_connection_reset_addrinfo(connection);

  if (connection->universal.con_list == connection)
    connection->universal.con_list= connection->next;
  if (connection->prev != NULL)
    connection->prev->next= connection->next;
  if (connection->next != NULL)
    connection->next->prev= connection->prev;
  connection->universal.con_count--;

  if (connection->options.packet_in_use)
    gearman_packet_free(&(connection->packet));

  delete connection;
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
    connection->options.ignore_lost_connection= value;
    break;
  case GEARMAN_CON_CLOSE_AFTER_FLUSH:
    connection->options.close_after_flush= value;
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

void gearman_connection_set_host(gearman_connection_st *connection,
                                 const char *host,
                                 in_port_t port)
{
  gearman_connection_reset_addrinfo(connection);

  strncpy(connection->host, host == NULL ? GEARMAN_DEFAULT_TCP_HOST : host,
          NI_MAXHOST);
  connection->host[NI_MAXHOST - 1]= 0;

  connection->port= in_port_t(port == 0 ? GEARMAN_DEFAULT_TCP_PORT : port);
}

void gearman_connection_close(gearman_connection_st *connection)
{
  if (connection->fd == INVALID_SOCKET)
    return;

  /* in case of death shutdown to avoid blocking at close() */
  if (shutdown(connection->fd, SHUT_RDWR) == SOCKET_ERROR && get_socket_errno() != ENOTCONN)
  {
    gearman_perror(connection->universal, "shutdown");
    assert(errno != ENOTSOCK);
    return;
  }

  if (closesocket(connection->fd) == SOCKET_ERROR)
  {
    gearman_perror(connection->universal, "close");
  }

  connection->state= GEARMAN_CON_UNIVERSAL_ADDRINFO;
  connection->fd= INVALID_SOCKET;
  connection->events= 0;
  connection->revents= 0;

  connection->send_state= GEARMAN_CON_SEND_STATE_NONE;
  connection->send_buffer_ptr= connection->send_buffer;
  connection->send_buffer_size= 0;
  connection->send_data_size= 0;
  connection->send_data_offset= 0;

  connection->recv_state= GEARMAN_CON_RECV_UNIVERSAL_NONE;
  if (connection->recv_packet != NULL)
  {
    gearman_packet_free(connection->recv_packet);
    connection->recv_packet= NULL;
  }

  connection->recv_buffer_ptr= connection->recv_buffer;
  connection->recv_buffer_size= 0;
}

void gearman_connection_reset_addrinfo(gearman_connection_st *connection)
{
  if (connection->addrinfo != NULL)
  {
    freeaddrinfo(connection->addrinfo);
    connection->addrinfo= NULL;
  }

  connection->addrinfo_next= NULL;
}

gearman_return_t gearman_connection_send(gearman_connection_st *connection,
                                         const gearman_packet_st *packet, bool flush)
{
  size_t send_size;

  switch (connection->send_state)
  {
  case GEARMAN_CON_SEND_STATE_NONE:
    if (not (packet->options.complete))
    {
      gearman_error(&connection->universal, GEARMAN_INVALID_PACKET, "packet not complete");
      return GEARMAN_INVALID_PACKET;
    }

    /* Pack first part of packet, which is everything but the payload. */
    while (1)
    {
      gearman_return_t rc;
      send_size= gearman_packet_pack(*packet,
                                     connection->send_buffer + connection->send_buffer_size,
                                     GEARMAN_SEND_BUFFER_SIZE -
                                     connection->send_buffer_size, rc);
      if (gearman_success(rc))
      {
        connection->send_buffer_size+= send_size;
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

      /* We were asked to flush when the buffer is already flushed! */
      if (connection->send_buffer_size == 0)
      {
        gearman_universal_set_error(&connection->universal, GEARMAN_SEND_BUFFER_TOO_SMALL, AT,
                                    "send buffer too small (%u)", GEARMAN_SEND_BUFFER_SIZE);
        return GEARMAN_SEND_BUFFER_TOO_SMALL;
      }

      /* Flush buffer now if first part of packet won't fit in. */
      connection->send_state= GEARMAN_CON_SEND_UNIVERSAL_PRE_FLUSH;

    case GEARMAN_CON_SEND_UNIVERSAL_PRE_FLUSH:
      {
        gearman_return_t ret= gearman_connection_flush(connection);
        if (gearman_failed(ret))
        {
          return ret;
        }
      }
    }

    /* Return here if we have no data to send. */
    if (not packet->data_size)
    {
      break;
    }

    /* If there is any room in the buffer, copy in data. */
    if (packet->data and
        (GEARMAN_SEND_BUFFER_SIZE - connection->send_buffer_size) > 0)
    {
      connection->send_data_offset= GEARMAN_SEND_BUFFER_SIZE - connection->send_buffer_size;
      if (connection->send_data_offset > packet->data_size)
        connection->send_data_offset= packet->data_size;

      memcpy(connection->send_buffer + connection->send_buffer_size, packet->data,
             connection->send_data_offset);
      connection->send_buffer_size+= connection->send_data_offset;

      /* Return if all data fit in the send buffer. */
      if (connection->send_data_offset == packet->data_size)
      {
        connection->send_data_offset= 0;
        break;
      }
    }

    /* Flush buffer now so we can start writing directly from data buffer. */
    connection->send_state= GEARMAN_CON_SEND_UNIVERSAL_FORCE_FLUSH;

  case GEARMAN_CON_SEND_UNIVERSAL_FORCE_FLUSH:
    {
      gearman_return_t ret= gearman_connection_flush(connection);
      if (gearman_failed(ret))
        return ret;
    }

    connection->send_data_size= packet->data_size;

    /* If this is NULL, then gearman_connection_send_data function will be used. */
    if (not packet->data)
    {
      connection->send_state= GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA;
      return GEARMAN_SUCCESS;
    }

    /* Copy into the buffer if it fits, otherwise flush from packet buffer. */
    connection->send_buffer_size= packet->data_size - connection->send_data_offset;
    if (connection->send_buffer_size < GEARMAN_SEND_BUFFER_SIZE)
    {
      memcpy(connection->send_buffer,
             static_cast<char *>(const_cast<void *>(packet->data)) + connection->send_data_offset,
             connection->send_buffer_size);
      connection->send_data_size= 0;
      connection->send_data_offset= 0;
      break;
    }

    connection->send_buffer_ptr= static_cast<char *>(const_cast<void *>(packet->data)) + connection->send_data_offset;
    connection->send_state= GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA;

  case GEARMAN_CON_SEND_UNIVERSAL_FLUSH:
  case GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA:
    {
      gearman_return_t ret= gearman_connection_flush(connection);
      if (gearman_success(ret) and connection->options.close_after_flush)
      {
        gearman_connection_close(connection);
        ret= GEARMAN_LOST_CONNECTION;
      }
      return ret;
    }
  }

  if (flush)
  {
    connection->send_state= GEARMAN_CON_SEND_UNIVERSAL_FLUSH;
    gearman_return_t ret= gearman_connection_flush(connection);
    if (ret == GEARMAN_SUCCESS && connection->options.close_after_flush)
    {
      gearman_connection_close(connection);
      ret= GEARMAN_LOST_CONNECTION;
    }
    return ret;
  }

  connection->send_state= GEARMAN_CON_SEND_STATE_NONE;
  return GEARMAN_SUCCESS;
}

size_t gearman_connection_send_data(gearman_connection_st *connection, const void *data,
                                    size_t data_size, gearman_return_t *ret_ptr)
{
  if (connection->send_state != GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA)
  {
    gearman_error(&connection->universal, GEARMAN_NOT_FLUSHING, "not flushing");
    return GEARMAN_NOT_FLUSHING;
  }

  if (data_size > (connection->send_data_size - connection->send_data_offset))
  {
    gearman_error(&connection->universal, GEARMAN_DATA_TOO_LARGE, "data too large");
    return GEARMAN_DATA_TOO_LARGE;
  }

  connection->send_buffer_ptr= static_cast<char *>(const_cast<void *>(data));
  connection->send_buffer_size= data_size;

  *ret_ptr= gearman_connection_flush(connection);

  return data_size - connection->send_buffer_size;
}

gearman_return_t gearman_connection_flush(gearman_connection_st *connection)
{
  while (1)
  {
    switch (connection->state)
    {
    case GEARMAN_CON_UNIVERSAL_ADDRINFO:
      {
        if (connection->addrinfo != NULL)
        {
          freeaddrinfo(connection->addrinfo);
          connection->addrinfo= NULL;
        }

        char port_str[NI_MAXSERV];
        snprintf(port_str, NI_MAXSERV, "%hu", uint16_t(connection->port));

        struct addrinfo ai;
        memset(&ai, 0, sizeof(struct addrinfo));
        ai.ai_socktype= SOCK_STREAM;
        ai.ai_protocol= IPPROTO_TCP;

        int ret= getaddrinfo(connection->host, port_str, &ai, &(connection->addrinfo));
        if (ret != 0)
        {
          gearman_universal_set_error(&connection->universal, GEARMAN_GETADDRINFO, AT, "getaddrinfo:%s",
                                      gai_strerror(ret));
          return GEARMAN_GETADDRINFO;
        }

        connection->addrinfo_next= connection->addrinfo;
      }

    case GEARMAN_CON_UNIVERSAL_CONNECT:
      if (connection->fd != -1)
        gearman_connection_close(connection);

      if (connection->addrinfo_next == NULL)
      {
        connection->state= GEARMAN_CON_UNIVERSAL_ADDRINFO;
        gearman_error(&connection->universal, GEARMAN_COULD_NOT_CONNECT, "could not connect");
        return GEARMAN_COULD_NOT_CONNECT;
      }

      connection->fd= socket(connection->addrinfo_next->ai_family,
                             connection->addrinfo_next->ai_socktype,
                             connection->addrinfo_next->ai_protocol);
      if (connection->fd == -1)
      {
        connection->state= GEARMAN_CON_UNIVERSAL_ADDRINFO;
        gearman_perror(connection->universal, "socket");
        return GEARMAN_ERRNO;
      }

      {
        gearman_return_t gret= _con_setsockopt(connection);
        if (gearman_failed(gret))
        {
          gearman_connection_close(connection);
          return gret;
        }
      }

      while (1)
      {
        int ret= connect(connection->fd, connection->addrinfo_next->ai_addr,
                         connection->addrinfo_next->ai_addrlen);
        if (ret == 0)
        {
          connection->state= GEARMAN_CON_UNIVERSAL_CONNECTED;
          connection->addrinfo_next= NULL;
          break;
        }

        if (errno == EAGAIN || errno == EINTR)
          continue;

        if (errno == EINPROGRESS)
        {
          connection->state= GEARMAN_CON_UNIVERSAL_CONNECTING;
          break;
        }

        if (errno == ECONNREFUSED || errno == ENETUNREACH || errno == ETIMEDOUT)
        {
          connection->state= GEARMAN_CON_UNIVERSAL_CONNECT;
          connection->addrinfo_next= connection->addrinfo_next->ai_next;
          break;
        }

        gearman_perror(connection->universal, "connect");
        gearman_connection_close(connection);
        return GEARMAN_ERRNO;
      }

      if (connection->state != GEARMAN_CON_UNIVERSAL_CONNECTING)
        break;

    case GEARMAN_CON_UNIVERSAL_CONNECTING:
      while (1)
      {
        if (connection->revents & (POLLERR | POLLHUP | POLLNVAL))
        {
          connection->state= GEARMAN_CON_UNIVERSAL_CONNECT;
          connection->addrinfo_next= connection->addrinfo_next->ai_next;
          break;
        }
        else if (connection->revents & POLLOUT)
        {
          connection->state= GEARMAN_CON_UNIVERSAL_CONNECTED;
          break;
        }

        gearman_return_t gret= gearman_connection_set_events(connection, POLLOUT);
        if (gearman_failed(gret))
          return gret;

        if (gearman_universal_is_non_blocking(connection->universal))
        {
          connection->state= GEARMAN_CON_UNIVERSAL_CONNECTING;
          gearman_gerror(&connection->universal, GEARMAN_IO_WAIT);
          return GEARMAN_IO_WAIT;
        }

        gret= gearman_wait(connection->universal);
        if (gearman_failed(gret))
          return gret;
      }

      if (connection->state != GEARMAN_CON_UNIVERSAL_CONNECTED)
        break;

    case GEARMAN_CON_UNIVERSAL_CONNECTED:
      while (connection->send_buffer_size != 0)
      {
#if 0
        {
          std::cerr << "send() " << connection->send_buffer_size << std::endl;
          if (connection->send_buffer_ptr[0] == 0)
          {
            for (size_t x= 1; x < connection->send_buffer_size; x++)
            {
              if (x == 1)
              {
                std::cerr.write(connection->send_buffer_ptr +x, 3);
                std::cerr << " ";
                x+= 2;
                continue;
              }

              if (x == 4)
              {
                uint32_t tmp;
                memcpy(&tmp, connection->send_buffer_ptr +x, sizeof(uint32_t));
                tmp= htonl(tmp);
                std::cerr << " " << gearman_strcommand(gearman_command_t(tmp)) << " ";
                x+= sizeof(uint32_t);

                memcpy(&tmp, connection->send_buffer_ptr +x, sizeof(uint32_t));
                tmp= htonl(tmp);
                std::cerr << " " << tmp << " ";
                x+= sizeof(uint32_t) -1;
                continue;
              }

              if (connection->send_buffer_ptr[x] == 0)
              {
                std::cerr <<  "<NULL>";
                continue;
              }
              std::cerr << char(connection->send_buffer_ptr[x]);
            }
          }
          std::cerr << std::endl;
        }
#endif
        ssize_t write_size= send(connection->fd, connection->send_buffer_ptr, connection->send_buffer_size, 
                                 gearman_universal_is_non_blocking(connection->universal) ? MSG_NOSIGNAL| MSG_DONTWAIT : MSG_NOSIGNAL);

        if (write_size == 0)
        {
          if (not (connection->options.ignore_lost_connection))
          {
            gearman_error(&connection->universal, GEARMAN_LOST_CONNECTION, "lost connection to server (EOF)");
          }
          gearman_connection_close(connection);
          return GEARMAN_LOST_CONNECTION;
        }
        else if (write_size == -1)
        {
          if (errno == EAGAIN)
          {
            gearman_return_t gret= gearman_connection_set_events(connection, POLLOUT);
            if (gearman_failed(gret))
              return gret;

            if (gearman_universal_is_non_blocking(connection->universal))
            {
              gearman_gerror(&connection->universal, GEARMAN_IO_WAIT);
              return GEARMAN_IO_WAIT;
            }

            gret= gearman_wait(connection->universal);
            if (gearman_failed(gret))
              return gret;

            continue;
          }
          else if (errno == EINTR)
          {
            continue;
          }
          else if (errno == EPIPE || errno == ECONNRESET || errno == EHOSTDOWN)
          {
            if (not (connection->options.ignore_lost_connection))
            {
              gearman_perror(connection->universal, "lost connection to server during send");
            }
            gearman_connection_close(connection);
            return GEARMAN_LOST_CONNECTION;
          }

          gearman_perror(connection->universal, "send");
          gearman_connection_close(connection);
          return GEARMAN_ERRNO;
        }

        connection->send_buffer_size-= size_t(write_size);
        if (connection->send_state == GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA)
        {
          connection->send_data_offset+= size_t(write_size);
          if (connection->send_data_offset == connection->send_data_size)
          {
            connection->send_data_size= 0;
            connection->send_data_offset= 0;
            break;
          }

          if (connection->send_buffer_size == 0)
            return GEARMAN_SUCCESS;
        }
        else if (connection->send_buffer_size == 0)
          break;

        connection->send_buffer_ptr+= write_size;
      }

      connection->send_state= GEARMAN_CON_SEND_STATE_NONE;
      connection->send_buffer_ptr= connection->send_buffer;
      return GEARMAN_SUCCESS;
    }
  }
}

gearman_packet_st *gearman_connection_recv(gearman_connection_st *connection,
                                           gearman_packet_st *packet,
                                           gearman_return_t *ret_ptr, bool recv_data)
{
  switch (connection->recv_state)
  {
  case GEARMAN_CON_RECV_UNIVERSAL_NONE:
    if (connection->state != GEARMAN_CON_UNIVERSAL_CONNECTED)
    {
      gearman_error(&connection->universal, GEARMAN_NOT_CONNECTED, "not connected");
      *ret_ptr= GEARMAN_NOT_CONNECTED;
      return NULL;
    }

    connection->recv_packet= gearman_packet_create(connection->universal, packet);
    if (not connection->recv_packet)
    {
      *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      return NULL;
    }

    connection->recv_state= GEARMAN_CON_RECV_UNIVERSAL_READ;

  case GEARMAN_CON_RECV_UNIVERSAL_READ:
    while (1)
    {
      if (connection->recv_buffer_size > 0)
      {
        size_t recv_size= gearman_packet_unpack(*connection->recv_packet,
                                                connection->recv_buffer_ptr,
                                                connection->recv_buffer_size, *ret_ptr);
        connection->recv_buffer_ptr+= recv_size;
        connection->recv_buffer_size-= recv_size;

        if (gearman_success(*ret_ptr))
        {
          break;
        }
        else if (*ret_ptr != GEARMAN_IO_WAIT)
        {
          gearman_connection_close(connection);
          return NULL;
        }
      }

      /* Shift buffer contents if needed. */
      if (connection->recv_buffer_size > 0)
        memmove(connection->recv_buffer, connection->recv_buffer_ptr, connection->recv_buffer_size);
      connection->recv_buffer_ptr= connection->recv_buffer;

      size_t recv_size= gearman_connection_read(connection, connection->recv_buffer + connection->recv_buffer_size,
                                                GEARMAN_RECV_BUFFER_SIZE - connection->recv_buffer_size, ret_ptr);
      if (gearman_failed(*ret_ptr))
      {
        return NULL;
      }

      connection->recv_buffer_size+= recv_size;
    }

    if (packet->data_size == 0)
    {
      connection->recv_state= GEARMAN_CON_RECV_UNIVERSAL_NONE;
      break;
    }

    connection->recv_data_size= packet->data_size;

    if (not recv_data)
    {
      connection->recv_state= GEARMAN_CON_RECV_STATE_READ_DATA;
      break;
    }

    if (packet->universal->workload_malloc_fn == NULL)
    {
      // Since it may be C on the other side, don't use new
      packet->data= malloc(packet->data_size);
    }
    else
    {
      packet->data= packet->universal->workload_malloc_fn(packet->data_size,
                                                        static_cast<void *>(packet->universal->workload_malloc_context));
    }
    if (packet->data == NULL)
    {
      *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      gearman_connection_close(connection);
      return NULL;
    }

    packet->options.free_data= true;
    connection->recv_state= GEARMAN_CON_RECV_STATE_READ_DATA;

  case GEARMAN_CON_RECV_STATE_READ_DATA:
    while (connection->recv_data_size != 0)
    {
      (void)gearman_connection_recv_data(connection,
                                         static_cast<uint8_t *>(const_cast<void *>(packet->data)) +
                                         connection->recv_data_offset,
                                         packet->data_size -
                                         connection->recv_data_offset, ret_ptr);
      if (gearman_failed(*ret_ptr))
      {
        return NULL;
      }
    }

    connection->recv_state= GEARMAN_CON_RECV_UNIVERSAL_NONE;
    break;
  }

  packet= connection->recv_packet;
  connection->recv_packet= NULL;

  return packet;
}

size_t gearman_connection_recv_data(gearman_connection_st *connection, void *data, size_t data_size,
                                    gearman_return_t *ret_ptr)
{
  size_t recv_size= 0;

  if (connection->recv_data_size == 0)
  {
    *ret_ptr= GEARMAN_SUCCESS;
    return 0;
  }

  if ((connection->recv_data_size - connection->recv_data_offset) < data_size)
    data_size= connection->recv_data_size - connection->recv_data_offset;

  if (connection->recv_buffer_size > 0)
  {
    if (connection->recv_buffer_size < data_size)
      recv_size= connection->recv_buffer_size;
    else
      recv_size= data_size;

    memcpy(data, connection->recv_buffer_ptr, recv_size);
    connection->recv_buffer_ptr+= recv_size;
    connection->recv_buffer_size-= recv_size;
  }

  if (data_size != recv_size)
  {
    recv_size+= gearman_connection_read(connection,
                                        static_cast<uint8_t *>(const_cast<void *>(data)) + recv_size,
                                        data_size - recv_size, ret_ptr);
    connection->recv_data_offset+= recv_size;
  }
  else
  {
    connection->recv_data_offset+= recv_size;
    *ret_ptr= GEARMAN_SUCCESS;
  }

  if (connection->recv_data_size == connection->recv_data_offset)
  {
    connection->recv_data_size= 0;
    connection->recv_data_offset= 0;
    connection->recv_state= GEARMAN_CON_RECV_UNIVERSAL_NONE;
  }

  return recv_size;
}

size_t gearman_connection_read(gearman_connection_st *connection, void *data, size_t data_size,
                               gearman_return_t *ret_ptr)
{
  ssize_t read_size;

  while (1)
  {
    read_size= read(connection->fd, data, data_size);
    if (read_size == 0)
    {
      if (not (connection->options.ignore_lost_connection))
      {
        gearman_error(&connection->universal, GEARMAN_LOST_CONNECTION, "lost connection to server (EOF)");
      }
      gearman_connection_close(connection);
      *ret_ptr= GEARMAN_LOST_CONNECTION;
      return 0;
    }
    else if (read_size == -1)
    {
      if (errno == EAGAIN)
      {
        *ret_ptr= gearman_connection_set_events(connection, POLLIN);
        if (*ret_ptr != GEARMAN_SUCCESS)
          return 0;

        if (gearman_universal_is_non_blocking(connection->universal))
        {
          gearman_gerror(&connection->universal, GEARMAN_IO_WAIT);
          *ret_ptr= GEARMAN_IO_WAIT;
          return 0;
        }

        *ret_ptr= gearman_wait(connection->universal);
        if (gearman_failed(*ret_ptr))
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
        if (not (connection->options.ignore_lost_connection))
        {
          gearman_perror(connection->universal, "lost connection to server during read");
        }
        *ret_ptr= GEARMAN_LOST_CONNECTION;
      }
      else
      {
        gearman_perror(connection->universal, "read");
        *ret_ptr= GEARMAN_ERRNO;
      }

      gearman_connection_close(connection);
      return 0;
    }

    break;
  }

  *ret_ptr= GEARMAN_SUCCESS;
  return size_t(read_size);
}

gearman_return_t gearman_connection_set_events(gearman_connection_st *connection, short events)
{
  if ((connection->events | events) == connection->events)
    return GEARMAN_SUCCESS;

  connection->events|= events;

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_connection_set_revents(gearman_connection_st *connection, short revents)
{
  if (revents != 0)
    connection->options.ready= true;

  connection->revents= revents;
  connection->events&= short(~revents);

  return GEARMAN_SUCCESS;
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
      gearman_perror(*connection->universal, "setsockopt(SO_NOSIGPIPE)");
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
