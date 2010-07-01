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

#include "common.h"

static void gearman_connection_reset_addrinfo(gearman_connection_st *connection);


/**
 * @addtogroup gearman_connection_static Static Connection Declarations
 * @ingroup gearman_connection
 * @{
 */


gearman_connection_st *gearman_connection_create(gearman_universal_st *gearman,
                                                 gearman_connection_st *connection,
                                                 gearman_connection_options_t *options)
{
  if (connection == NULL)
  {
    connection= malloc(sizeof(gearman_connection_st));
    if (connection == NULL)
    {
      gearman_universal_set_error(gearman, "gearman_connection_create", "malloc");
      return NULL;
    }

    connection->options.allocated= true;
  }
  else
  {
    connection->options.allocated= false;
  }

  connection->options.ready= false;
  connection->options.packet_in_use= false;
  connection->options.external_fd= false;
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


  connection->state= 0;
  connection->send_state= 0;
  connection->recv_state= 0;
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
  connection->universal= gearman;

  if (gearman->con_list != NULL)
    gearman->con_list->prev= connection;
  connection->next= gearman->con_list;
  connection->prev= NULL;
  gearman->con_list= connection;
  gearman->con_count++;

  connection->context= NULL;
  connection->addrinfo= NULL;
  connection->addrinfo_next= NULL;
  connection->send_buffer_ptr= connection->send_buffer;
  connection->recv_packet= NULL;
  connection->recv_buffer_ptr= connection->recv_buffer;
  connection->protocol_context= NULL;
  connection->protocol_context_free_fn= NULL;
  connection->packet_pack_fn= gearman_packet_pack;
  connection->packet_unpack_fn= gearman_packet_unpack;
  connection->host[0]= 0;

  return connection;
}

gearman_connection_st *gearman_connection_create_args(gearman_universal_st *gearman, gearman_connection_st *connection,
                                                      const char *host, in_port_t port)
{
  connection= gearman_connection_create(gearman, connection, NULL);
  if (connection == NULL)
    return NULL;

  gearman_connection_set_host(connection, host, port);

  return connection;
}

gearman_connection_st *gearman_connection_clone(gearman_universal_st *gearman, gearman_connection_st *connection,
                                                const gearman_connection_st *from)
{
  connection= gearman_connection_create(gearman, connection, NULL);

  if (from == NULL || connection == NULL)
    return connection;

  connection->options.ready= from->options.ready;
  connection->options.packet_in_use= from->options.packet_in_use;
  connection->options.external_fd= from->options.external_fd;
  connection->options.ignore_lost_connection= from->options.ignore_lost_connection;
  connection->options.close_after_flush= from->options.close_after_flush;

  strcpy(connection->host, from->host);
  connection->port= from->port;

  return connection;
}

void gearman_connection_free(gearman_connection_st *connection)
{
  if (connection->fd != -1)
    gearman_connection_close(connection);

  gearman_connection_reset_addrinfo(connection);

  if (connection->protocol_context != NULL && connection->protocol_context_free_fn != NULL)
    connection->protocol_context_free_fn(connection, (void *)connection->protocol_context);

  if (connection->universal->con_list == connection)
    connection->universal->con_list= connection->next;
  if (connection->prev != NULL)
    connection->prev->next= connection->next;
  if (connection->next != NULL)
    connection->next->prev= connection->prev;
  connection->universal->con_count--;

  if (connection->options.packet_in_use)
    gearman_packet_free(&(connection->packet));

  if (connection->options.allocated)
    free(connection);
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
  case GEARMAN_CON_EXTERNAL_FD:
    connection->options.external_fd= value;
    break;
  case GEARMAN_CON_IGNORE_LOST_CONNECTION:
    connection->options.ignore_lost_connection= value;
    break;
  case GEARMAN_CON_CLOSE_AFTER_FLUSH:
    connection->options.close_after_flush= value;
    break;
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

  connection->port= (in_port_t)(port == 0 ? GEARMAN_DEFAULT_TCP_PORT : port);
}

gearman_return_t gearman_connection_set_fd(gearman_connection_st *connection, int fd)
{
  gearman_return_t ret;

  connection->options.external_fd= true;
  connection->fd= fd;
  connection->state= GEARMAN_CON_UNIVERSAL_CONNECTED;

  ret= _con_setsockopt(connection);
  if (ret != GEARMAN_SUCCESS)
  {
    connection->universal->last_errno= errno;
    return ret;
  }

  return GEARMAN_SUCCESS;
}

void *gearman_connection_context(const gearman_connection_st *connection)
{
  return connection->context;
}

void gearman_connection_set_context(gearman_connection_st *connection, void *context)
{
  connection->context= context;
}

gearman_return_t gearman_connection_connect(gearman_connection_st *connection)
{
  return gearman_connection_flush(connection);
}

void gearman_connection_close(gearman_connection_st *connection)
{
  if (connection->fd == -1)
    return;

  if (connection->options.external_fd)
    connection->options.external_fd= false;
  else
    (void)close(connection->fd);

  connection->state= GEARMAN_CON_UNIVERSAL_ADDRINFO;
  connection->fd= -1;
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
  gearman_return_t ret;
  size_t send_size;

  switch (connection->send_state)
  {
  case GEARMAN_CON_SEND_STATE_NONE:
    if (! (packet->options.complete))
    {
      gearman_universal_set_error(connection->universal, "gearman_connection_send",
                        "packet not complete");
      return GEARMAN_INVALID_PACKET;
    }

    /* Pack first part of packet, which is everything but the payload. */
    while (1)
    {
      send_size= connection->packet_pack_fn(packet, connection,
                                     connection->send_buffer + connection->send_buffer_size,
                                     GEARMAN_SEND_BUFFER_SIZE -
                                     connection->send_buffer_size, &ret);
      if (ret == GEARMAN_SUCCESS)
      {
        connection->send_buffer_size+= send_size;
        break;
      }
      else if (ret == GEARMAN_IGNORE_PACKET)
        return GEARMAN_SUCCESS;
      else if (ret != GEARMAN_FLUSH_DATA)
        return ret;

      /* We were asked to flush when the buffer is already flushed! */
      if (connection->send_buffer_size == 0)
      {
        gearman_universal_set_error(connection->universal, "gearman_connection_send",
                          "send buffer too small (%u)",
                          GEARMAN_SEND_BUFFER_SIZE);
        return GEARMAN_SEND_BUFFER_TOO_SMALL;
      }

      /* Flush buffer now if first part of packet won't fit in. */
      connection->send_state= GEARMAN_CON_SEND_UNIVERSAL_PRE_FLUSH;

    case GEARMAN_CON_SEND_UNIVERSAL_PRE_FLUSH:
      ret= gearman_connection_flush(connection);
      if (ret != GEARMAN_SUCCESS)
        return ret;
    }

    /* Return here if we have no data to send. */
    if (packet->data_size == 0)
      break;

    /* If there is any room in the buffer, copy in data. */
    if (packet->data != NULL &&
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
    ret= gearman_connection_flush(connection);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    connection->send_data_size= packet->data_size;

    /* If this is NULL, then gearman_connection_send_data function will be used. */
    if (packet->data == NULL)
    {
      connection->send_state= GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA;
      return GEARMAN_SUCCESS;
    }

    /* Copy into the buffer if it fits, otherwise flush from packet buffer. */
    connection->send_buffer_size= packet->data_size - connection->send_data_offset;
    if (connection->send_buffer_size < GEARMAN_SEND_BUFFER_SIZE)
    {
      memcpy(connection->send_buffer,
             (char *)packet->data + connection->send_data_offset,
             connection->send_buffer_size);
      connection->send_data_size= 0;
      connection->send_data_offset= 0;
      break;
    }

    connection->send_buffer_ptr= (char *)packet->data + connection->send_data_offset;
    connection->send_state= GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA;

  case GEARMAN_CON_SEND_UNIVERSAL_FLUSH:
  case GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA:
    ret= gearman_connection_flush(connection);
    if (ret == GEARMAN_SUCCESS && connection->options.close_after_flush)
    {
      gearman_connection_close(connection);
      ret= GEARMAN_LOST_CONNECTION;
    }
    return ret;

  default:
    gearman_universal_set_error(connection->universal, "gearman_connection_send", "unknown state: %u",
                      connection->send_state);
    return GEARMAN_UNKNOWN_STATE;
  }

  if (flush)
  {
    connection->send_state= GEARMAN_CON_SEND_UNIVERSAL_FLUSH;
    ret= gearman_connection_flush(connection);
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
    gearman_universal_set_error(connection->universal, "gearman_connection_send_data", "not flushing");
    return GEARMAN_NOT_FLUSHING;
  }

  if (data_size > (connection->send_data_size - connection->send_data_offset))
  {
    gearman_universal_set_error(connection->universal, "gearman_connection_send_data", "data too large");
    return GEARMAN_DATA_TOO_LARGE;
  }

  connection->send_buffer_ptr= (char *)data;
  connection->send_buffer_size= data_size;

  *ret_ptr= gearman_connection_flush(connection);

  return data_size - connection->send_buffer_size;
}

gearman_return_t gearman_connection_flush(gearman_connection_st *connection)
{
  char port_str[NI_MAXSERV];
  struct addrinfo ai;
  int ret;
  ssize_t write_size;
  gearman_return_t gret;

  while (1)
  {
    switch (connection->state)
    {
    case GEARMAN_CON_UNIVERSAL_ADDRINFO:
      if (connection->addrinfo != NULL)
      {
        freeaddrinfo(connection->addrinfo);
        connection->addrinfo= NULL;
      }

      snprintf(port_str, NI_MAXSERV, "%hu", (uint16_t)connection->port);

      memset(&ai, 0, sizeof(struct addrinfo));
      ai.ai_socktype= SOCK_STREAM;
      ai.ai_protocol= IPPROTO_TCP;

      ret= getaddrinfo(connection->host, port_str, &ai, &(connection->addrinfo));
      if (ret != 0)
      {
        gearman_universal_set_error(connection->universal, "gearman_connection_flush", "getaddrinfo:%s",
                          gai_strerror(ret));
        return GEARMAN_GETADDRINFO;
      }

      connection->addrinfo_next= connection->addrinfo;

    case GEARMAN_CON_UNIVERSAL_CONNECT:
      if (connection->fd != -1)
        gearman_connection_close(connection);

      if (connection->addrinfo_next == NULL)
      {
        connection->state= GEARMAN_CON_UNIVERSAL_ADDRINFO;
        gearman_universal_set_error(connection->universal, "gearman_connection_flush",
                          "could not connect");
        return GEARMAN_COULD_NOT_CONNECT;
      }

      connection->fd= socket(connection->addrinfo_next->ai_family,
                      connection->addrinfo_next->ai_socktype,
                      connection->addrinfo_next->ai_protocol);
      if (connection->fd == -1)
      {
        connection->state= GEARMAN_CON_UNIVERSAL_ADDRINFO;
        gearman_universal_set_error(connection->universal, "gearman_connection_flush", "socket:%d",
                          errno);
        connection->universal->last_errno= errno;
        return GEARMAN_ERRNO;
      }

      gret= _con_setsockopt(connection);
      if (gret != GEARMAN_SUCCESS)
      {
        connection->universal->last_errno= errno;
        gearman_connection_close(connection);
        return gret;
      }

      while (1)
      {
        ret= connect(connection->fd, connection->addrinfo_next->ai_addr,
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

        gearman_universal_set_error(connection->universal, "gearman_connection_flush", "connect:%d",
                          errno);
        connection->universal->last_errno= errno;
        gearman_connection_close(connection);
        return GEARMAN_ERRNO;
      }

      if (connection->state != GEARMAN_CON_UNIVERSAL_CONNECTING)
        break;

    case GEARMAN_CON_UNIVERSAL_CONNECTING:
      while (1)
      {
        if (connection->revents & POLLOUT)
        {
          connection->state= GEARMAN_CON_UNIVERSAL_CONNECTED;
          break;
        }
        else if (connection->revents & (POLLERR | POLLHUP | POLLNVAL))
        {
          connection->state= GEARMAN_CON_UNIVERSAL_CONNECT;
          connection->addrinfo_next= connection->addrinfo_next->ai_next;
          break;
        }

        gret= gearman_connection_set_events(connection, POLLOUT);
        if (gret != GEARMAN_SUCCESS)
          return gret;

        if (gearman_universal_is_non_blocking(connection->universal))
        {
          connection->state= GEARMAN_CON_UNIVERSAL_CONNECTING;
          return GEARMAN_IO_WAIT;
        }

        gret= gearman_wait(connection->universal);
        if (gret != GEARMAN_SUCCESS)
          return gret;
      }

      if (connection->state != GEARMAN_CON_UNIVERSAL_CONNECTED)
        break;

    case GEARMAN_CON_UNIVERSAL_CONNECTED:
      while (connection->send_buffer_size != 0)
      {
        write_size= write(connection->fd, connection->send_buffer_ptr, connection->send_buffer_size);
        if (write_size == 0)
        {
          if (! (connection->options.ignore_lost_connection))
          {
            gearman_universal_set_error(connection->universal, "gearman_connection_flush",
                              "lost connection to server (EOF)");
          }
          gearman_connection_close(connection);
          return GEARMAN_LOST_CONNECTION;
        }
        else if (write_size == -1)
        {
          if (errno == EAGAIN)
          {
            gret= gearman_connection_set_events(connection, POLLOUT);
            if (gret != GEARMAN_SUCCESS)
              return gret;

            if (gearman_universal_is_non_blocking(connection->universal))
              return GEARMAN_IO_WAIT;

            gret= gearman_wait(connection->universal);
            if (gret != GEARMAN_SUCCESS)
              return gret;

            continue;
          }
          else if (errno == EINTR)
            continue;
          else if (errno == EPIPE || errno == ECONNRESET || errno == EHOSTDOWN)
          {
            if (! (connection->options.ignore_lost_connection))
            {
              gearman_universal_set_error(connection->universal, "gearman_connection_flush",
                                "lost connection to server (%d)", errno);
            }
            gearman_connection_close(connection);
            return GEARMAN_LOST_CONNECTION;
          }

          gearman_universal_set_error(connection->universal, "gearman_connection_flush", "write:%d",
                            errno);
          connection->universal->last_errno= errno;
          gearman_connection_close(connection);
          return GEARMAN_ERRNO;
        }

        connection->send_buffer_size-= (size_t)write_size;
        if (connection->send_state == GEARMAN_CON_SEND_UNIVERSAL_FLUSH_DATA)
        {
          connection->send_data_offset+= (size_t)write_size;
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

    default:
      gearman_universal_set_error(connection->universal, "gearman_connection_flush", "unknown state: %u",
                        connection->state);

      return GEARMAN_UNKNOWN_STATE;
    }
  }
}

gearman_packet_st *gearman_connection_recv(gearman_connection_st *connection,
                                           gearman_packet_st *packet,
                                           gearman_return_t *ret_ptr, bool recv_data)
{
  size_t recv_size;

  switch (connection->recv_state)
  {
  case GEARMAN_CON_RECV_UNIVERSAL_NONE:
    if (connection->state != GEARMAN_CON_UNIVERSAL_CONNECTED)
    {
      gearman_universal_set_error(connection->universal, "gearman_connection_recv", "not connected");
      *ret_ptr= GEARMAN_NOT_CONNECTED;
      return NULL;
    }

    connection->recv_packet= gearman_packet_create(connection->universal, packet);
    if (connection->recv_packet == NULL)
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
        recv_size= connection->packet_unpack_fn(connection->recv_packet, connection,
                                         connection->recv_buffer_ptr,
                                         connection->recv_buffer_size, ret_ptr);
        connection->recv_buffer_ptr+= recv_size;
        connection->recv_buffer_size-= recv_size;
        if (*ret_ptr == GEARMAN_SUCCESS)
          break;
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

      recv_size= gearman_connection_read(connection, connection->recv_buffer + connection->recv_buffer_size,
                                         GEARMAN_RECV_BUFFER_SIZE - connection->recv_buffer_size,
                                         ret_ptr);
      if (*ret_ptr != GEARMAN_SUCCESS)
        return NULL;

      connection->recv_buffer_size+= recv_size;
    }

    if (packet->data_size == 0)
    {
      connection->recv_state= GEARMAN_CON_RECV_UNIVERSAL_NONE;
      break;
    }

    connection->recv_data_size= packet->data_size;

    if (!recv_data)
    {
      connection->recv_state= GEARMAN_CON_RECV_STATE_READ_DATA;
      break;
    }

    if (packet->universal->workload_malloc_fn == NULL)
    {
      packet->data= malloc(packet->data_size);
    }
    else
    {
      packet->data= packet->universal->workload_malloc_fn(packet->data_size,
                                                        (void *)packet->universal->workload_malloc_context);
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
                                         ((uint8_t *)(packet->data)) +
                                         connection->recv_data_offset,
                                         packet->data_size -
                                         connection->recv_data_offset, ret_ptr);
      if (*ret_ptr != GEARMAN_SUCCESS)
        return NULL;
    }

    connection->recv_state= GEARMAN_CON_RECV_UNIVERSAL_NONE;
    break;

  default:
    gearman_universal_set_error(connection->universal, "gearman_connection_recv", "unknown state: %u",
                      connection->recv_state);
    *ret_ptr= GEARMAN_UNKNOWN_STATE;
    return NULL;
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
    recv_size+= gearman_connection_read(connection, ((uint8_t *)data) + recv_size,
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
      if (! (connection->options.ignore_lost_connection))
      {
        gearman_universal_set_error(connection->universal, "gearman_connection_read",
                          "lost connection to server (EOF)");
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
          *ret_ptr= GEARMAN_IO_WAIT;
          return 0;
        }

        *ret_ptr= gearman_wait(connection->universal);
        if (*ret_ptr != GEARMAN_SUCCESS)
          return 0;

        continue;
      }
      else if (errno == EINTR)
        continue;
      else if (errno == EPIPE || errno == ECONNRESET || errno == EHOSTDOWN)
      {
        if (! (connection->options.ignore_lost_connection))
        {
          gearman_universal_set_error(connection->universal, "gearman_connection_read",
                            "lost connection to server (%d)", errno);
        }
        *ret_ptr= GEARMAN_LOST_CONNECTION;
      }
      else
      {
        gearman_universal_set_error(connection->universal, "gearman_connection_read", "read:%d", errno);
        connection->universal->last_errno= errno;
        *ret_ptr= GEARMAN_ERRNO;
      }

      gearman_connection_close(connection);
      return 0;
    }

    break;
  }

  *ret_ptr= GEARMAN_SUCCESS;
  return (size_t)read_size;
}

gearman_return_t gearman_connection_set_events(gearman_connection_st *connection, short events)
{
  gearman_return_t ret;

  if ((connection->events | events) == connection->events)
    return GEARMAN_SUCCESS;

  connection->events|= events;

  if (connection->universal->event_watch_fn != NULL)
  {
    ret= connection->universal->event_watch_fn(connection, connection->events,
                                      (void *)connection->universal->event_watch_context);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_connection_close(connection);
      return ret;
    }
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_connection_set_revents(gearman_connection_st *connection, short revents)
{
  gearman_return_t ret;

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
    ret= connection->universal->event_watch_fn(connection, connection->events,
                                      (void *)connection->universal->event_watch_context);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_connection_close(connection);
      return ret;
    }
  }

  connection->events&= (short)~revents;

  return GEARMAN_SUCCESS;
}

void *gearman_connection_protocol_context(const gearman_connection_st *connection)
{
  return connection->protocol_context;
}

void gearman_connection_set_protocol_context(gearman_connection_st *connection, void *context)
{
  connection->protocol_context= context;
}

void gearman_connection_set_protocol_context_free_fn(gearman_connection_st *connection,
                                                     gearman_connection_protocol_context_free_fn *function)
{
  connection->protocol_context_free_fn= function;
}

void gearman_connection_set_packet_pack_fn(gearman_connection_st *connection,
                                           gearman_packet_pack_fn *function)
{
  connection->packet_pack_fn= function;
}

void gearman_connection_set_packet_unpack_fn(gearman_connection_st *connection,
                                             gearman_packet_unpack_fn *function)
{
  connection->packet_unpack_fn= function;
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
                  (socklen_t)sizeof(int));
  if (ret == -1 && errno != EOPNOTSUPP)
  {
    gearman_universal_set_error(connection->universal, "_con_setsockopt",
                                "setsockopt:TCP_NODELAY:%d", errno);
    return GEARMAN_ERRNO;
  }

  linger.l_onoff= 1;
  linger.l_linger= GEARMAN_DEFAULT_SOCKET_TIMEOUT;
  ret= setsockopt(connection->fd, SOL_SOCKET, SO_LINGER, &linger,
                  (socklen_t)sizeof(struct linger));
  if (ret == -1)
  {
    gearman_universal_set_error(connection->universal, "_con_setsockopt",
                                "setsockopt:SO_LINGER:%d", errno);
    return GEARMAN_ERRNO;
  }

  waittime.tv_sec= GEARMAN_DEFAULT_SOCKET_TIMEOUT;
  waittime.tv_usec= 0;
  ret= setsockopt(connection->fd, SOL_SOCKET, SO_SNDTIMEO, &waittime,
                  (socklen_t)sizeof(struct timeval));
  if (ret == -1 && errno != ENOPROTOOPT)
  {
    gearman_universal_set_error(connection->universal, "_con_setsockopt",
                                "setsockopt:SO_SNDTIMEO:%d", errno);
    return GEARMAN_ERRNO;
  }

  ret= setsockopt(connection->fd, SOL_SOCKET, SO_RCVTIMEO, &waittime,
                  (socklen_t)sizeof(struct timeval));
  if (ret == -1 && errno != ENOPROTOOPT)
  {
    gearman_universal_set_error(connection->universal, "_con_setsockopt",
                                "setsockopt:SO_RCVTIMEO:%d", errno);
    return GEARMAN_ERRNO;
  }

  ret= GEARMAN_DEFAULT_SOCKET_SEND_SIZE;
  ret= setsockopt(connection->fd, SOL_SOCKET, SO_SNDBUF, &ret, (socklen_t)sizeof(int));
  if (ret == -1)
  {
    gearman_universal_set_error(connection->universal, "_con_setsockopt",
                                "setsockopt:SO_SNDBUF:%d", errno);
    return GEARMAN_ERRNO;
  }

  ret= GEARMAN_DEFAULT_SOCKET_RECV_SIZE;
  ret= setsockopt(connection->fd, SOL_SOCKET, SO_RCVBUF, &ret, (socklen_t)sizeof(int));
  if (ret == -1)
  {
    gearman_universal_set_error(connection->universal, "_con_setsockopt",
                                "setsockopt:SO_RCVBUF:%d", errno);
    return GEARMAN_ERRNO;
  }

  ret= fcntl(connection->fd, F_GETFL, 0);
  if (ret == -1)
  {
    gearman_universal_set_error(connection->universal, "_con_setsockopt", "fcntl:F_GETFL:%d",
                                errno);
    return GEARMAN_ERRNO;
  }

  ret= fcntl(connection->fd, F_SETFL, ret | O_NONBLOCK);
  if (ret == -1)
  {
    gearman_universal_set_error(connection->universal, "_con_setsockopt", "fcntl:F_SETFL:%d",
                                errno);
    return GEARMAN_ERRNO;
  }

  return GEARMAN_SUCCESS;
}
