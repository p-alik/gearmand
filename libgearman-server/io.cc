/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Connection Definitions
 */

#include <config.h>
#include <libgearman-server/common.h>
#include <libgearman-server/plugins/base.h>

#include <cstring>
#include <cerrno>
#include <cassert>

static void _connection_close(gearmand_io_st *connection)
{
  if (connection->fd == INVALID_SOCKET)
  {
    return;
  }

  if (connection->options.external_fd)
  {
    connection->options.external_fd= false;
  }
  else
  {
    (void)gearmand_sockfd_close(connection->fd);
    assert(! "We should never have an internal fd");
  }

  connection->_state= gearmand_io_st::GEARMAND_CON_UNIVERSAL_INVALID;
  connection->fd= INVALID_SOCKET;
  connection->events= 0;
  connection->revents= 0;

  connection->send_state= gearmand_io_st::GEARMAND_CON_SEND_STATE_NONE;
  connection->send_buffer_ptr= connection->send_buffer;
  connection->send_buffer_size= 0;
  connection->send_data_size= 0;
  connection->send_data_offset= 0;

  connection->recv_state= gearmand_io_st::GEARMAND_CON_RECV_UNIVERSAL_NONE;
  if (connection->recv_packet != NULL)
  {
    gearmand_packet_free(connection->recv_packet);
    connection->recv_packet= NULL;
  }

  connection->recv_buffer_ptr= connection->recv_buffer;
  connection->recv_buffer_size= 0;
}

static size_t _connection_read(gearman_server_con_st *con, void *data, size_t data_size, gearmand_error_t &ret)
{
  ssize_t read_size;
  gearmand_io_st *connection= &con->con;

  while (1)
  {
    read_size= recv(connection->fd, data, data_size, MSG_DONTWAIT);

    if (read_size == 0)
    {
      ret= GEARMAN_LOST_CONNECTION;
      gearmand_log_info(GEARMAN_DEFAULT_LOG_PARAM, 
                        "Peer connection has called close() %s:%s",
                        connection->context == NULL ? "-" : connection->context->host,
                        connection->context == NULL ? "-" : connection->context->port);
      _connection_close(connection);
      return 0;
    }
    else if (read_size == -1)
    {
      switch (errno)
      {
      case EAGAIN:
        ret= gearmand_io_set_events(con, POLLIN);
        if (gearmand_failed(ret))
        {
          gearmand_perror("recv");
          return 0;
        }

        ret= GEARMAN_IO_WAIT;
        return 0;

      case EINTR:
        continue;

      case EPIPE:
      case ECONNRESET:
      case EHOSTDOWN:
        gearmand_perror("lost connection to client recv(EPIPE || ECONNRESET || EHOSTDOWN)");
        ret= GEARMAN_LOST_CONNECTION;
        break;

      default:
        gearmand_perror("recv");
        ret= GEARMAN_ERRNO;
      }

      gearmand_error("closing connection due to previous errno error");
      _connection_close(connection);
      return 0;
    }

    break;
  }

  ret= GEARMAN_SUCCESS;
  return size_t(read_size);
}

gearmand_error_t gearmand_connection_recv_data(gearman_server_con_st *con, void *data, size_t data_size)
{
  gearmand_io_st *connection= &con->con;

  if (connection->recv_data_size == 0)
  {
    return GEARMAN_SUCCESS;
  }

  if ((connection->recv_data_size - connection->recv_data_offset) < data_size)
  {
    data_size= connection->recv_data_size - connection->recv_data_offset;
  }

  size_t recv_size= 0;
  if (connection->recv_buffer_size > 0)
  {
    if (connection->recv_buffer_size < data_size)
    {
      recv_size= connection->recv_buffer_size;
    }
    else
    {
      recv_size= data_size;
    }

    memcpy(data, connection->recv_buffer_ptr, recv_size);
    connection->recv_buffer_ptr+= recv_size;
    connection->recv_buffer_size-= recv_size;
  }

  gearmand_error_t ret;
  if (data_size != recv_size)
  {
    recv_size+= _connection_read(con, ((uint8_t *)data) + recv_size, data_size - recv_size, ret);
    connection->recv_data_offset+= recv_size;
  }
  else
  {
    connection->recv_data_offset+= recv_size;
    ret= GEARMAN_SUCCESS;
  }

  if (connection->recv_data_size == connection->recv_data_offset)
  {
    connection->recv_data_size= 0;
    connection->recv_data_offset= 0;
    connection->recv_state= gearmand_io_st::GEARMAND_CON_RECV_UNIVERSAL_NONE;
  }

  return ret;
}

static gearmand_error_t _connection_flush(gearman_server_con_st *con)
{
  gearmand_io_st *connection= &con->con;

  assert(connection->_state == gearmand_io_st::GEARMAND_CON_UNIVERSAL_CONNECTED);
  while (1)
  {
    switch (connection->_state)
    {
    case gearmand_io_st::GEARMAND_CON_UNIVERSAL_INVALID:
      assert(0);
      return GEARMAN_ERRNO;

    case gearmand_io_st::GEARMAND_CON_UNIVERSAL_CONNECTED:
      while (connection->send_buffer_size)
      {
        ssize_t write_size= send(connection->fd, connection->send_buffer_ptr, connection->send_buffer_size, MSG_NOSIGNAL|MSG_DONTWAIT);

        if (write_size == 0) // detect infinite loop?
        {
          gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "send() sent zero bytes to peer %s:%s",
                             connection->context == NULL ? "-" : connection->context->host,
                             connection->context == NULL ? "-" : connection->context->port);
          continue;
        }
        else if (write_size == -1)
        {
          switch (errno)
          {
          case EAGAIN:
            {
              gearmand_error_t gret= gearmand_io_set_events(con, POLLOUT);
              if (gret != GEARMAN_SUCCESS)
              {
                return gret;
              }
              return GEARMAN_IO_WAIT;
            }

          case EINTR:
            continue;

          case EPIPE:
          case ECONNRESET:
          case EHOSTDOWN:
            gearmand_perror("lost connection to client during send(EPIPE || ECONNRESET || EHOSTDOWN)");
            _connection_close(connection);
            return GEARMAN_LOST_CONNECTION;

          default:
            break;
          }

          gearmand_perror("send() failed, closing connection");
          _connection_close(connection);
          return GEARMAN_ERRNO;
        }

        gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "send() %u bytes to peer %s:%s",
                           uint32_t(write_size),
                           connection->context == NULL ? "-" : connection->context->host,
                           connection->context == NULL ? "-" : connection->context->port);

        connection->send_buffer_size-= static_cast<size_t>(write_size);
        if (connection->send_state == gearmand_io_st::GEARMAND_CON_SEND_UNIVERSAL_FLUSH_DATA)
        {
          connection->send_data_offset+= static_cast<size_t>(write_size);
          if (connection->send_data_offset == connection->send_data_size)
          {
            connection->send_data_size= 0;
            connection->send_data_offset= 0;
            break;
          }

          if (connection->send_buffer_size == 0)
          {
            return GEARMAN_SUCCESS;
          }
        }
        else if (connection->send_buffer_size == 0)
        {
          break;
        }

        connection->send_buffer_ptr+= write_size;
      }

      connection->send_state= gearmand_io_st::GEARMAND_CON_SEND_STATE_NONE;
      connection->send_buffer_ptr= connection->send_buffer;
      return GEARMAN_SUCCESS;
    }
  }
}

/**
 * @addtogroup gearmand_io_static Static Connection Declarations
 * @ingroup gearman_connection
 * @{
 */


void gearmand_connection_init(gearmand_connection_list_st *gearman,
                              gearmand_io_st *connection,
                              gearmand_con_st *dcon,
                              gearmand_connection_options_t *options)
{
  assert(gearman);
  assert(connection);

  connection->options.ready= false;
  connection->options.packet_in_use= false;
  connection->options.external_fd= false;
  connection->options.close_after_flush= false;

  if (options)
  {
    while (*options != GEARMAND_CON_MAX)
    {
      gearman_io_set_option(connection, *options, true);
      options++;
    }
  }


  connection->_state= gearmand_io_st::GEARMAND_CON_UNIVERSAL_INVALID;
  connection->send_state= gearmand_io_st::GEARMAND_CON_SEND_STATE_NONE;
  connection->recv_state= gearmand_io_st::GEARMAND_CON_RECV_UNIVERSAL_NONE;
  connection->events= 0;
  connection->revents= 0;
  connection->fd= INVALID_SOCKET;
  connection->created_id= 0;
  connection->created_id_next= 0;
  connection->send_buffer_size= 0;
  connection->send_data_size= 0;
  connection->send_data_offset= 0;
  connection->recv_buffer_size= 0;
  connection->recv_data_size= 0;
  connection->recv_data_offset= 0;
  connection->universal= gearman;

  if (gearman->con_list != NULL)
    gearman->con_list->prev= connection;
  connection->next= gearman->con_list;
  connection->prev= NULL;
  gearman->con_list= connection;
  gearman->con_count++;

  connection->context= dcon;

  connection->send_buffer_ptr= connection->send_buffer;
  connection->recv_packet= NULL;
  connection->recv_buffer_ptr= connection->recv_buffer;
}

void gearmand_io_free(gearmand_io_st *connection)
{
  if (connection->fd != INVALID_SOCKET)
    _connection_close(connection);

  {
    if (connection->universal->con_list == connection)
      connection->universal->con_list= connection->next;

    if (connection->prev != NULL)
      connection->prev->next= connection->next;

    if (connection->next != NULL)
      connection->next->prev= connection->prev;
    connection->universal->con_count--;
  }

  if (connection->options.packet_in_use)
  {
    gearmand_packet_free(&(connection->packet));
  }
}

gearmand_error_t gearman_io_set_option(gearmand_io_st *connection,
                                               gearmand_connection_options_t options,
                                               bool value)
{
  switch (options)
  {
  case GEARMAND_CON_READY:
    connection->options.ready= value;
    break;
  case GEARMAND_CON_PACKET_IN_USE:
    connection->options.packet_in_use= value;
    break;
  case GEARMAND_CON_EXTERNAL_FD:
    connection->options.external_fd= value;
    break;
  case GEARMAND_CON_CLOSE_AFTER_FLUSH:
    connection->options.close_after_flush= value;
    break;
  case GEARMAND_CON_MAX:
    return GEARMAN_INVALID_COMMAND;
  }

  return GEARMAN_SUCCESS;
}

/**
 * Set socket options for a connection.
 */
static gearmand_error_t _io_setsockopt(gearmand_io_st &connection);

/** @} */

/*
 * Public Definitions
 */

gearmand_error_t gearman_io_set_fd(gearmand_io_st *connection, int fd)
{
  assert(connection);

  connection->options.external_fd= true;
  connection->fd= fd;
  connection->_state= gearmand_io_st::GEARMAND_CON_UNIVERSAL_CONNECTED;

  return _io_setsockopt(*connection);
}

gearmand_con_st *gearman_io_context(const gearmand_io_st *connection)
{
  return connection->context;
}

gearmand_error_t gearman_io_send(gearman_server_con_st *con,
                                 const gearmand_packet_st *packet, bool flush)
{
  gearmand_error_t ret;
  size_t send_size;

  gearmand_io_st *connection= &con->con;

  switch (connection->send_state)
  {
  case gearmand_io_st::GEARMAND_CON_SEND_STATE_NONE:
    if (! (packet->options.complete))
    {
      gearmand_error("packet not complete");
      return GEARMAN_INVALID_PACKET;
    }

    /* Pack first part of packet, which is everything but the payload. */
    while (1)
    {
      send_size= con->protocol->pack(packet,
                                     con,
                                     connection->send_buffer +connection->send_buffer_size,
                                     GEARMAN_SEND_BUFFER_SIZE -connection->send_buffer_size,
                                     ret);
      if (ret == GEARMAN_SUCCESS)
      {
        connection->send_buffer_size+= send_size;
        break;
      }
      else if (ret == GEARMAN_IGNORE_PACKET)
      {
        return GEARMAN_SUCCESS;
      }
      else if (ret != GEARMAN_FLUSH_DATA)
      {
        return ret;
      }

      /* We were asked to flush when the buffer is already flushed! */
      if (connection->send_buffer_size == 0)
      {
        gearmand_error("send buffer too small");

        return GEARMAN_SEND_BUFFER_TOO_SMALL;
      }

      /* Flush buffer now if first part of packet won't fit in. */
      connection->send_state= gearmand_io_st::GEARMAND_CON_SEND_UNIVERSAL_PRE_FLUSH;

    case gearmand_io_st::GEARMAND_CON_SEND_UNIVERSAL_PRE_FLUSH:
      if ((ret= _connection_flush(con)) != GEARMAN_SUCCESS)
      {
        return ret;
      }
    }

    /* Return here if we have no data to send. */
    if (packet->data_size == 0)
    {
      break;
    }

    /* If there is any room in the buffer, copy in data. */
    if (packet->data and (GEARMAN_SEND_BUFFER_SIZE - connection->send_buffer_size) > 0)
    {
      connection->send_data_offset= GEARMAN_SEND_BUFFER_SIZE - connection->send_buffer_size;
      if (connection->send_data_offset > packet->data_size)
      {
        connection->send_data_offset= packet->data_size;
      }

      memcpy(connection->send_buffer +connection->send_buffer_size,
             packet->data,
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
    connection->send_state= gearmand_io_st::GEARMAND_CON_SEND_UNIVERSAL_FORCE_FLUSH;

  case gearmand_io_st::GEARMAND_CON_SEND_UNIVERSAL_FORCE_FLUSH:
    if ((ret= _connection_flush(con)) != GEARMAN_SUCCESS)
    {
      return ret;
    }

    connection->send_data_size= packet->data_size;

    /* If this is NULL, then ?? function will be used. */
    if (packet->data == NULL)
    {
      connection->send_state= gearmand_io_st::GEARMAND_CON_SEND_UNIVERSAL_FLUSH_DATA;
      return GEARMAN_SUCCESS;
    }

    /* Copy into the buffer if it fits, otherwise flush from packet buffer. */
    connection->send_buffer_size= packet->data_size - connection->send_data_offset;
    if (connection->send_buffer_size < GEARMAN_SEND_BUFFER_SIZE)
    {
      memcpy(connection->send_buffer,
             packet->data + connection->send_data_offset,
             connection->send_buffer_size);
      connection->send_data_size= 0;
      connection->send_data_offset= 0;
      break;
    }

    connection->send_buffer_ptr= const_cast<char *>(packet->data) + connection->send_data_offset;
    connection->send_state= gearmand_io_st::GEARMAND_CON_SEND_UNIVERSAL_FLUSH_DATA;

  case gearmand_io_st::GEARMAND_CON_SEND_UNIVERSAL_FLUSH:
  case gearmand_io_st::GEARMAND_CON_SEND_UNIVERSAL_FLUSH_DATA:
    ret= _connection_flush(con);
    if (ret == GEARMAN_SUCCESS and
        connection->options.close_after_flush)
    {
      _connection_close(connection);
      ret= GEARMAN_LOST_CONNECTION;
      gearmand_debug("closing connection after flush by request");
    }
    return ret;
  }

  if (flush)
  {
    connection->send_state= gearmand_io_st::GEARMAND_CON_SEND_UNIVERSAL_FLUSH;
    ret= _connection_flush(con);
    if (ret == GEARMAN_SUCCESS and connection->options.close_after_flush)
    {
      _connection_close(connection);
      ret= GEARMAN_LOST_CONNECTION;
      gearmand_debug("closing connection after flush by request");
    }
    return ret;
  }

  connection->send_state= gearmand_io_st::GEARMAND_CON_SEND_STATE_NONE;
  return GEARMAN_SUCCESS;
}

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif


gearmand_error_t gearman_io_recv(gearman_server_con_st *con, bool recv_data)
{
  gearmand_io_st *connection= &con->con;
  gearmand_packet_st *packet= &(con->packet->packet);

  switch (connection->recv_state)
  {
  case gearmand_io_st::GEARMAND_CON_RECV_UNIVERSAL_NONE:
    if (connection->_state != gearmand_io_st::GEARMAND_CON_UNIVERSAL_CONNECTED)
    {
      gearmand_error("not connected");
      return GEARMAN_NOT_CONNECTED;
    }

    connection->recv_packet= packet;
    // The options being passed in are just defaults.
    gearmand_packet_init(connection->recv_packet, GEARMAN_MAGIC_TEXT, GEARMAN_COMMAND_TEXT);

    connection->recv_state= gearmand_io_st::GEARMAND_CON_RECV_UNIVERSAL_READ;

  case gearmand_io_st::GEARMAND_CON_RECV_UNIVERSAL_READ:
    while (1)
    {
      gearmand_error_t ret;

      if (connection->recv_buffer_size > 0)
      {
        assert(con->protocol);
        size_t recv_size= con->protocol->unpack(connection->recv_packet,
                                                con,
                                                connection->recv_buffer_ptr,
                                                connection->recv_buffer_size, ret);
        connection->recv_buffer_ptr+= recv_size;
        connection->recv_buffer_size-= recv_size;
        if (gearmand_success(ret))
        {
          break;
        }
        else if (ret != GEARMAN_IO_WAIT)
        {
	  gearmand_gerror_warn("protocol failure, closing connection", ret);
          _connection_close(connection);
          return ret;
        }
      }

      /* Shift buffer contents if needed. */
      if (connection->recv_buffer_size > 0)
      {
        memmove(connection->recv_buffer, connection->recv_buffer_ptr, connection->recv_buffer_size);
      }
      connection->recv_buffer_ptr= connection->recv_buffer;

      size_t recv_size= _connection_read(con, connection->recv_buffer + connection->recv_buffer_size,
					 GEARMAN_RECV_BUFFER_SIZE - connection->recv_buffer_size, ret);
      if (gearmand_failed(ret))
      {
        // GEARMAN_LOST_CONNECTION is not worth a warning, clients/workers just
        // drop connections for close.
        if (ret != GEARMAN_LOST_CONNECTION)
        {
          gearmand_gerror_warn("Failed while in _connection_read()", ret);
        }
        return ret;
      }
      gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, "read %lu bytes", (unsigned long)recv_size);

      connection->recv_buffer_size+= recv_size;
    }

    if (packet->data_size == 0)
    {
      connection->recv_state= gearmand_io_st::GEARMAND_CON_RECV_UNIVERSAL_NONE;
      break;
    }

    connection->recv_data_size= packet->data_size;

    if (not recv_data)
    {
      connection->recv_state= gearmand_io_st::GEARMAND_CON_RECV_STATE_READ_DATA;
      break;
    }

    packet->data= static_cast<char *>(malloc(packet->data_size));
    if (not packet->data)
    {
      // Server up the memory error first, in case _connection_close()
      // creates any.
      gearmand_merror("malloc", char, packet->data_size);
      _connection_close(connection);
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    packet->options.free_data= true;
    connection->recv_state= gearmand_io_st::GEARMAND_CON_RECV_STATE_READ_DATA;

  case gearmand_io_st::GEARMAND_CON_RECV_STATE_READ_DATA:
    while (connection->recv_data_size)
    {
      gearmand_error_t ret;
      ret= gearmand_connection_recv_data(con,
                                         ((uint8_t *)(packet->data)) +
                                         connection->recv_data_offset,
                                         packet->data_size -
                                         connection->recv_data_offset);
      if (gearmand_failed(ret))
      {
        return ret;
      }
    }

    connection->recv_state= gearmand_io_st::GEARMAND_CON_RECV_UNIVERSAL_NONE;
    break;
  }

  packet= connection->recv_packet;
  connection->recv_packet= NULL;

  return GEARMAN_SUCCESS;
}

gearmand_error_t gearmand_io_set_events(gearman_server_con_st *con, short events)
{
  gearmand_io_st *connection= &con->con;

  if ((connection->events | events) == connection->events)
  {
    return GEARMAN_SUCCESS;
  }

  connection->events|= events;

  if (connection->universal->event_watch_fn)
  {
    gearmand_error_t ret= connection->universal->event_watch_fn(connection, connection->events,
                                                                (void *)connection->universal->event_watch_context);
    if (gearmand_failed(ret))
    {
      gearmand_gerror_warn("event watch failed, closing connection", ret);
      _connection_close(connection);
      return ret;
    }
  }

  return GEARMAN_SUCCESS;
}

gearmand_error_t gearmand_io_set_revents(gearman_server_con_st *con, short revents)
{
  gearmand_io_st *connection= &con->con;

  if (revents != 0)
    connection->options.ready= true;

  connection->revents= revents;

  /* Remove external POLLOUT watch if we didn't ask for it. Otherwise we spin
    forever until another POLLIN state change. This is much more efficient
    than removing POLLOUT on every state change since some external polling
    mechanisms need to use a system call to change flags (like Linux epoll). */
  if (revents & POLLOUT && !(connection->events & POLLOUT) &&
      connection->universal->event_watch_fn != NULL)
  {
    gearmand_error_t ret= connection->universal->event_watch_fn(connection, connection->events,
                                                                (void *)connection->universal->event_watch_context);
    if (gearmand_failed(ret))
    {
      gearmand_gerror_warn("event watch failed, closing connection", ret);
      _connection_close(connection);
      return ret;
    }
  }

  connection->events&= (short)~revents;

  return GEARMAN_SUCCESS;
}

/*
 * Static Definitions
 */

static gearmand_error_t _io_setsockopt(gearmand_io_st &connection)
{
  struct linger linger;
  struct timeval waittime;

  int setting= 1;
  if (setsockopt(connection.fd, IPPROTO_TCP, TCP_NODELAY, &setting, (socklen_t)sizeof(int)) and errno != EOPNOTSUPP)
  {
    gearmand_perror("setsockopt(TCP_NODELAY)");
    return GEARMAN_ERRNO;
  }

  linger.l_onoff= 1;
  linger.l_linger= GEARMAN_DEFAULT_SOCKET_TIMEOUT;
  if (setsockopt(connection.fd, SOL_SOCKET, SO_LINGER, &linger, (socklen_t)sizeof(struct linger)))
  {
    gearmand_perror("setsockopt(SO_LINGER)");
    return GEARMAN_ERRNO;
  }

#if defined(__MACH__) && defined(__APPLE__) || defined(__FreeBSD__)
  {
    setting= 1;

    // This is not considered a fatal error 
    if (setsockopt(connection.fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&setting, sizeof(int)))
    {
      gearmand_perror("setsockopt(SO_NOSIGPIPE)");
    }
  }
#endif

  waittime.tv_sec= GEARMAN_DEFAULT_SOCKET_TIMEOUT;
  waittime.tv_usec= 0;
  if (setsockopt(connection.fd, SOL_SOCKET, SO_SNDTIMEO, &waittime, (socklen_t)sizeof(struct timeval)) and errno != ENOPROTOOPT)
  {
    gearmand_perror("setsockopt(SO_SNDTIMEO)");
    return GEARMAN_ERRNO;
  }

  if (setsockopt(connection.fd, SOL_SOCKET, SO_RCVTIMEO, &waittime, (socklen_t)sizeof(struct timeval)) and errno != ENOPROTOOPT)
  {
    gearmand_error("setsockopt(SO_RCVTIMEO)");
    return GEARMAN_ERRNO;
  }

  setting= GEARMAN_DEFAULT_SOCKET_SEND_SIZE;
  if (setsockopt(connection.fd, SOL_SOCKET, SO_SNDBUF, &setting, (socklen_t)sizeof(int)))
  {
    gearmand_perror("setsockopt(SO_SNDBUF)");
    return GEARMAN_ERRNO;
  }

  setting= GEARMAN_DEFAULT_SOCKET_RECV_SIZE;
  if (setsockopt(connection.fd, SOL_SOCKET, SO_RCVBUF, &setting, (socklen_t)sizeof(int)))
  {
    gearmand_perror("setsockopt(SO_RCVBUF)");
    return GEARMAN_ERRNO;
  }

  int fcntl_flags;
  if ((fcntl_flags= fcntl(connection.fd, F_GETFL, 0)) == -1)
  {
    gearmand_perror("fcntl(F_GETFL)");
    return GEARMAN_ERRNO;
  }

  if ((fcntl(connection.fd, F_SETFL, fcntl_flags | O_NONBLOCK) == -1))
  {
    gearmand_perror("fcntl(F_SETFL)");
    return GEARMAN_ERRNO;
  }

  return GEARMAN_SUCCESS;
}

void gearmand_sockfd_close(int sockfd)
{
  if (sockfd == INVALID_SOCKET)
    return;

  /* in case of death shutdown to avoid blocking at close() */
  if (shutdown(sockfd, SHUT_RDWR) == SOCKET_ERROR && get_socket_errno() != ENOTCONN)
  {
    gearmand_perror("shutdown");
    assert(errno != ENOTSOCK);
    return;
  }

  if (closesocket(sockfd) == SOCKET_ERROR)
  {
    gearmand_perror("close");
  }
}

void gearmand_pipe_close(int pipefd)
{
  if (pipefd == INVALID_SOCKET)
  {
    return;
  }

  if (closesocket(pipefd) == SOCKET_ERROR)
  {
    gearmand_perror("close");
  }
}
