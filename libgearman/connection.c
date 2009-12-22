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

/**
 * @addtogroup gearman_connection_static Static Connection Declarations
 * @ingroup gearman_con
 * @{
 */


gearman_connection_st *gearman_connection_create(gearman_state_st *gearman, gearman_connection_st *con)
{
  if (con == NULL)
  {
    con= malloc(sizeof(gearman_connection_st));
    if (con == NULL)
    {
      gearman_set_error(gearman, "gearman_connection_create", "malloc");
      return NULL;
    }

    con->options= GEARMAN_CON_ALLOCATED;
  }
  else
    con->options= 0;

  con->state= 0;
  con->send_state= 0;
  con->recv_state= 0;
  con->port= 0;
  con->events= 0;
  con->revents= 0;
  con->fd= -1;
  con->created_id= 0;
  con->created_id_next= 0;
  con->send_buffer_size= 0;
  con->send_data_size= 0;
  con->send_data_offset= 0;
  con->recv_buffer_size= 0;
  con->recv_data_size= 0;
  con->recv_data_offset= 0;
  con->gearman= gearman;

  if (gearman->con_list != NULL)
    gearman->con_list->prev= con;
  con->next= gearman->con_list;
  con->prev= NULL;
  gearman->con_list= con;
  gearman->con_count++;

  con->context= NULL;
  con->addrinfo= NULL;
  con->addrinfo_next= NULL;
  con->send_buffer_ptr= con->send_buffer;
  con->recv_packet= NULL;
  con->recv_buffer_ptr= con->recv_buffer;
  con->protocol_context= NULL;
  con->protocol_context_free_fn= NULL;
  con->packet_pack_fn= gearman_packet_pack;
  con->packet_unpack_fn= gearman_packet_unpack;
  con->host[0]= 0;

  return con;
}

gearman_connection_st *gearman_connection_create_args(gearman_state_st *gearman, gearman_connection_st *con,
                                                      const char *host, in_port_t port)
{
  con= gearman_connection_create(gearman, con);
  if (con == NULL)
    return NULL;

  gearman_connection_set_host(con, host);
  gearman_connection_set_port(con, port);

  return con;
}

gearman_connection_st *gearman_connection_clone(gearman_state_st *gearman, gearman_connection_st *con,
                                                const gearman_connection_st *from)
{
  con= gearman_connection_create(gearman, con);
  if (con == NULL)
    return NULL;

  con->options|= (from->options &
                  (gearman_connection_options_t)~GEARMAN_CON_ALLOCATED);
  strcpy(con->host, from->host);
  con->port= from->port;

  return con;
}

void gearman_connection_free(gearman_connection_st *con)
{
  if (con->fd != -1)
    gearman_connection_close(con);

  gearman_connection_reset_addrinfo(con);

  if (con->protocol_context != NULL && con->protocol_context_free_fn != NULL)
    con->protocol_context_free_fn(con, (void *)con->protocol_context);

  if (con->gearman->con_list == con)
    con->gearman->con_list= con->next;
  if (con->prev != NULL)
    con->prev->next= con->next;
  if (con->next != NULL)
    con->next->prev= con->prev;
  con->gearman->con_count--;

  if (con->options & GEARMAN_CON_PACKET_IN_USE)
    gearman_packet_free(&(con->packet));

  if (con->options & GEARMAN_CON_ALLOCATED)
    free(con);
}

/**
 * Set socket options for a connection.
 */
static gearman_return_t _con_setsockopt(gearman_connection_st *con);

/** @} */

/*
 * Public Definitions
 */

void gearman_connection_set_host(gearman_connection_st *con, const char *host)
{
  gearman_connection_reset_addrinfo(con);

  strncpy(con->host, host == NULL ? GEARMAN_DEFAULT_TCP_HOST : host,
          NI_MAXHOST);
  con->host[NI_MAXHOST - 1]= 0;
}

void gearman_connection_set_port(gearman_connection_st *con, in_port_t port)
{
  gearman_connection_reset_addrinfo(con);

  con->port= (in_port_t)(port == 0 ? GEARMAN_DEFAULT_TCP_PORT : port);
}

gearman_connection_options_t gearman_connection_options(const gearman_connection_st *con)
{
  return con->options;
}

void gearman_connection_set_options(gearman_connection_st *con,
                                    gearman_connection_options_t options)
{
  con->options= options;
}

void gearman_connection_add_options(gearman_connection_st *con,
                                    gearman_connection_options_t options)
{
  con->options|= options;
}

void gearman_connection_remove_options(gearman_connection_st *con,
                                       gearman_connection_options_t options)
{
  con->options&= ~options;
}

gearman_return_t gearman_connection_set_fd(gearman_connection_st *con, int fd)
{
  gearman_return_t ret;

  con->options|= GEARMAN_CON_EXTERNAL_FD;
  con->fd= fd;
  con->state= GEARMAN_CON_STATE_CONNECTED;

  ret= _con_setsockopt(con);
  if (ret != GEARMAN_SUCCESS)
  {
    con->gearman->last_errno= errno;
    return ret;
  }

  return GEARMAN_SUCCESS;
}

void *gearman_connection_context(const gearman_connection_st *con)
{
  return (void *)con->context;
}

void gearman_connection_set_context(gearman_connection_st *con, const void *context)
{
  con->context= context;
}

gearman_return_t gearman_connection_connect(gearman_connection_st *con)
{
  return gearman_connection_flush(con);
}

void gearman_connection_close(gearman_connection_st *con)
{
  if (con->fd == -1)
    return;

  if (con->options & GEARMAN_CON_EXTERNAL_FD)
    con->options&= (gearman_connection_options_t)~GEARMAN_CON_EXTERNAL_FD;
  else
    (void)close(con->fd);

  con->state= GEARMAN_CON_STATE_ADDRINFO;
  con->fd= -1;
  con->events= 0;
  con->revents= 0;

  con->send_state= GEARMAN_CON_SEND_STATE_NONE;
  con->send_buffer_ptr= con->send_buffer;
  con->send_buffer_size= 0;
  con->send_data_size= 0;
  con->send_data_offset= 0;

  con->recv_state= GEARMAN_CON_RECV_STATE_NONE;
  if (con->recv_packet != NULL)
    gearman_packet_free(con->recv_packet);
  con->recv_buffer_ptr= con->recv_buffer;
  con->recv_buffer_size= 0;
}

void gearman_connection_reset_addrinfo(gearman_connection_st *con)
{
  if (con->addrinfo != NULL)
  {
    freeaddrinfo(con->addrinfo);
    con->addrinfo= NULL;
  }

  con->addrinfo_next= NULL;
}

gearman_return_t gearman_connection_send(gearman_connection_st *con,
                                         const gearman_packet_st *packet, bool flush)
{
  gearman_return_t ret;
  size_t send_size;

  switch (con->send_state)
  {
  case GEARMAN_CON_SEND_STATE_NONE:
    if (! (packet->options.complete))
    {
      gearman_set_error(con->gearman, "gearman_connection_send",
                        "packet not complete");
      return GEARMAN_INVALID_PACKET;
    }

    /* Pack first part of packet, which is everything but the payload. */
    while (1)
    {
      send_size= con->packet_pack_fn(packet, con,
                                     con->send_buffer + con->send_buffer_size,
                                     GEARMAN_SEND_BUFFER_SIZE -
                                     con->send_buffer_size, &ret);
      if (ret == GEARMAN_SUCCESS)
      {
        con->send_buffer_size+= send_size;
        break;
      }
      else if (ret == GEARMAN_IGNORE_PACKET)
        return GEARMAN_SUCCESS;
      else if (ret != GEARMAN_FLUSH_DATA)
        return ret;

      /* We were asked to flush when the buffer is already flushed! */
      if (con->send_buffer_size == 0)
      {
        gearman_set_error(con->gearman, "gearman_connection_send",
                          "send buffer too small (%u)",
                          GEARMAN_SEND_BUFFER_SIZE);
        return GEARMAN_SEND_BUFFER_TOO_SMALL;
      }

      /* Flush buffer now if first part of packet won't fit in. */
      con->send_state= GEARMAN_CON_SEND_STATE_PRE_FLUSH;

    case GEARMAN_CON_SEND_STATE_PRE_FLUSH:
      ret= gearman_connection_flush(con);
      if (ret != GEARMAN_SUCCESS)
        return ret;
    }

    /* Return here if we have no data to send. */
    if (packet->data_size == 0)
      break;

    /* If there is any room in the buffer, copy in data. */
    if (packet->data != NULL &&
        (GEARMAN_SEND_BUFFER_SIZE - con->send_buffer_size) > 0)
    {
      con->send_data_offset= GEARMAN_SEND_BUFFER_SIZE - con->send_buffer_size;
      if (con->send_data_offset > packet->data_size)
        con->send_data_offset= packet->data_size;

      memcpy(con->send_buffer + con->send_buffer_size, packet->data,
             con->send_data_offset);
      con->send_buffer_size+= con->send_data_offset;

      /* Return if all data fit in the send buffer. */
      if (con->send_data_offset == packet->data_size)
      {
        con->send_data_offset= 0;
        break;
      }
    }

    /* Flush buffer now so we can start writing directly from data buffer. */
    con->send_state= GEARMAN_CON_SEND_STATE_FORCE_FLUSH;

  case GEARMAN_CON_SEND_STATE_FORCE_FLUSH:
    ret= gearman_connection_flush(con);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    con->send_data_size= packet->data_size;

    /* If this is NULL, then gearman_connection_send_data function will be used. */
    if (packet->data == NULL)
    {
      con->send_state= GEARMAN_CON_SEND_STATE_FLUSH_DATA;
      return GEARMAN_SUCCESS;
    }

    /* Copy into the buffer if it fits, otherwise flush from packet buffer. */
    con->send_buffer_size= packet->data_size - con->send_data_offset;
    if (con->send_buffer_size < GEARMAN_SEND_BUFFER_SIZE)
    {
      memcpy(con->send_buffer,
             ((uint8_t *)(packet->data)) + con->send_data_offset,
             con->send_buffer_size);
      con->send_data_size= 0;
      con->send_data_offset= 0;
      break;
    }

    con->send_buffer_ptr= ((uint8_t *)(packet->data)) + con->send_data_offset;
    con->send_state= GEARMAN_CON_SEND_STATE_FLUSH_DATA;

  case GEARMAN_CON_SEND_STATE_FLUSH:
  case GEARMAN_CON_SEND_STATE_FLUSH_DATA:
    ret= gearman_connection_flush(con);
    if (ret == GEARMAN_SUCCESS && con->options & GEARMAN_CON_CLOSE_AFTER_FLUSH)
    {
      gearman_connection_close(con);
      ret= GEARMAN_LOST_CONNECTION;
    }
    return ret;

  default:
    gearman_set_error(con->gearman, "gearman_connection_send", "unknown state: %u",
                      con->send_state);
    return GEARMAN_UNKNOWN_STATE;
  }

  if (flush)
  {
    con->send_state= GEARMAN_CON_SEND_STATE_FLUSH;
    ret= gearman_connection_flush(con);
    if (ret == GEARMAN_SUCCESS && con->options & GEARMAN_CON_CLOSE_AFTER_FLUSH)
    {
      gearman_connection_close(con);
      ret= GEARMAN_LOST_CONNECTION;
    }
    return ret;
  }

  con->send_state= GEARMAN_CON_SEND_STATE_NONE;
  return GEARMAN_SUCCESS;
}

size_t gearman_connection_send_data(gearman_connection_st *con, const void *data,
                                    size_t data_size, gearman_return_t *ret_ptr)
{
  if (con->send_state != GEARMAN_CON_SEND_STATE_FLUSH_DATA)
  {
    gearman_set_error(con->gearman, "gearman_connection_send_data", "not flushing");
    return GEARMAN_NOT_FLUSHING;
  }

  if (data_size > (con->send_data_size - con->send_data_offset))
  {
    gearman_set_error(con->gearman, "gearman_connection_send_data", "data too large");
    return GEARMAN_DATA_TOO_LARGE;
  }

  con->send_buffer_ptr= (uint8_t *)data;
  con->send_buffer_size= data_size;

  *ret_ptr= gearman_connection_flush(con);

  return data_size - con->send_buffer_size;
}

gearman_return_t gearman_connection_flush(gearman_connection_st *con)
{
  char port_str[NI_MAXSERV];
  struct addrinfo ai;
  int ret;
  ssize_t write_size;
  gearman_return_t gret;

  while (1)
  {
    switch (con->state)
    {
    case GEARMAN_CON_STATE_ADDRINFO:
      if (con->addrinfo != NULL)
      {
        freeaddrinfo(con->addrinfo);
        con->addrinfo= NULL;
      }

      snprintf(port_str, NI_MAXSERV, "%u", con->port);

      memset(&ai, 0, sizeof(struct addrinfo));
      ai.ai_socktype= SOCK_STREAM;
      ai.ai_protocol= IPPROTO_TCP;

      ret= getaddrinfo(con->host, port_str, &ai, &(con->addrinfo));
      if (ret != 0)
      {
        gearman_set_error(con->gearman, "gearman_connection_flush", "getaddrinfo:%s",
                          gai_strerror(ret));
        return GEARMAN_GETADDRINFO;
      }

      con->addrinfo_next= con->addrinfo;

    case GEARMAN_CON_STATE_CONNECT:
      if (con->fd != -1)
        gearman_connection_close(con);

      if (con->addrinfo_next == NULL)
      {
        con->state= GEARMAN_CON_STATE_ADDRINFO;
        gearman_set_error(con->gearman, "gearman_connection_flush",
                          "could not connect");
        return GEARMAN_COULD_NOT_CONNECT;
      }

      con->fd= socket(con->addrinfo_next->ai_family,
                      con->addrinfo_next->ai_socktype,
                      con->addrinfo_next->ai_protocol);
      if (con->fd == -1)
      {
        con->state= GEARMAN_CON_STATE_ADDRINFO;
        gearman_set_error(con->gearman, "gearman_connection_flush", "socket:%d",
                          errno);
        con->gearman->last_errno= errno;
        return GEARMAN_ERRNO;
      }

      gret= _con_setsockopt(con);
      if (gret != GEARMAN_SUCCESS)
      {
        con->gearman->last_errno= errno;
        gearman_connection_close(con);
        return gret;
      }

      while (1)
      {
        ret= connect(con->fd, con->addrinfo_next->ai_addr,
                     con->addrinfo_next->ai_addrlen);
        if (ret == 0)
        {
          con->state= GEARMAN_CON_STATE_CONNECTED;
          con->addrinfo_next= NULL;
          break;
        }

        if (errno == EAGAIN || errno == EINTR)
          continue;

        if (errno == EINPROGRESS)
        {
          con->state= GEARMAN_CON_STATE_CONNECTING;
          break;
        }

        if (errno == ECONNREFUSED || errno == ENETUNREACH || errno == ETIMEDOUT)
        {
          con->state= GEARMAN_CON_STATE_CONNECT;
          con->addrinfo_next= con->addrinfo_next->ai_next;
          break;
        }

        gearman_set_error(con->gearman, "gearman_connection_flush", "connect:%d",
                          errno);
        con->gearman->last_errno= errno;
        gearman_connection_close(con);
        return GEARMAN_ERRNO;
      }

      if (con->state != GEARMAN_CON_STATE_CONNECTING)
        break;

    case GEARMAN_CON_STATE_CONNECTING:
      while (1)
      {
        if (con->revents & POLLOUT)
        {
          con->state= GEARMAN_CON_STATE_CONNECTED;
          break;
        }
        else if (con->revents & (POLLERR | POLLHUP | POLLNVAL))
        {
          con->state= GEARMAN_CON_STATE_CONNECT;
          con->addrinfo_next= con->addrinfo_next->ai_next;
          break;
        }

        gret= gearman_connection_set_events(con, POLLOUT);
        if (gret != GEARMAN_SUCCESS)
          return gret;

        if (gearman_state_is_non_blocking(con->gearman))
        {
          con->state= GEARMAN_CON_STATE_CONNECTING;
          return GEARMAN_IO_WAIT;
        }

        gret= gearman_wait(con->gearman);
        if (gret != GEARMAN_SUCCESS)
          return gret;
      }

      if (con->state != GEARMAN_CON_STATE_CONNECTED)
        break;

    case GEARMAN_CON_STATE_CONNECTED:
      while (con->send_buffer_size != 0)
      {
        write_size= write(con->fd, con->send_buffer_ptr, con->send_buffer_size);
        if (write_size == 0)
        {
          if (!(con->options & GEARMAN_CON_IGNORE_LOST_CONNECTION))
          {
            gearman_set_error(con->gearman, "gearman_connection_flush",
                              "lost connection to server (EOF)");
          }
          gearman_connection_close(con);
          return GEARMAN_LOST_CONNECTION;
        }
        else if (write_size == -1)
        {
          if (errno == EAGAIN)
          {
            gret= gearman_connection_set_events(con, POLLOUT);
            if (gret != GEARMAN_SUCCESS)
              return gret;

            if (gearman_state_is_non_blocking(con->gearman))
              return GEARMAN_IO_WAIT;

            gret= gearman_wait(con->gearman);
            if (gret != GEARMAN_SUCCESS)
              return gret;

            continue;
          }
          else if (errno == EINTR)
            continue;
          else if (errno == EPIPE || errno == ECONNRESET)
          {
            if (!(con->options & GEARMAN_CON_IGNORE_LOST_CONNECTION))
            {
              gearman_set_error(con->gearman, "gearman_connection_flush",
                                "lost connection to server (%d)", errno);
            }
            gearman_connection_close(con);
            return GEARMAN_LOST_CONNECTION;
          }

          gearman_set_error(con->gearman, "gearman_connection_flush", "write:%d",
                            errno);
          con->gearman->last_errno= errno;
          gearman_connection_close(con);
          return GEARMAN_ERRNO;
        }

        con->send_buffer_size-= (size_t)write_size;
        if (con->send_state == GEARMAN_CON_SEND_STATE_FLUSH_DATA)
        {
          con->send_data_offset+= (size_t)write_size;
          if (con->send_data_offset == con->send_data_size)
          {
            con->send_data_size= 0;
            con->send_data_offset= 0;
            break;
          }

          if (con->send_buffer_size == 0)
            return GEARMAN_SUCCESS;
        }
        else if (con->send_buffer_size == 0)
          break;

        con->send_buffer_ptr+= write_size;
      }

      con->send_state= GEARMAN_CON_SEND_STATE_NONE;
      con->send_buffer_ptr= con->send_buffer;
      return GEARMAN_SUCCESS;

    default:
      gearman_set_error(con->gearman, "gearman_connection_flush", "unknown state: %u",
                        con->state);

      return GEARMAN_UNKNOWN_STATE;
    }
  }
}

gearman_packet_st *gearman_connection_recv(gearman_connection_st *con,
                                           gearman_packet_st *packet,
                                           gearman_return_t *ret_ptr, bool recv_data)
{
  size_t recv_size;

  switch (con->recv_state)
  {
  case GEARMAN_CON_RECV_STATE_NONE:
    if (con->state != GEARMAN_CON_STATE_CONNECTED)
    {
      gearman_set_error(con->gearman, "gearman_connection_recv", "not connected");
      *ret_ptr= GEARMAN_NOT_CONNECTED;
      return NULL;
    }

    con->recv_packet= gearman_packet_create(con->gearman, packet);
    if (con->recv_packet == NULL)
    {
      *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      return NULL;
    }

    con->recv_state= GEARMAN_CON_RECV_STATE_READ;

  case GEARMAN_CON_RECV_STATE_READ:
    while (1)
    {
      if (con->recv_buffer_size > 0)
      {
        recv_size= con->packet_unpack_fn(con->recv_packet, con,
                                         con->recv_buffer_ptr,
                                         con->recv_buffer_size, ret_ptr);
        con->recv_buffer_ptr+= recv_size;
        con->recv_buffer_size-= recv_size;
        if (*ret_ptr == GEARMAN_SUCCESS)
          break;
        else if (*ret_ptr != GEARMAN_IO_WAIT)
        {
          gearman_connection_close(con);
          return NULL;
        }
      }

      /* Shift buffer contents if needed. */
      if (con->recv_buffer_size > 0)
        memmove(con->recv_buffer, con->recv_buffer_ptr, con->recv_buffer_size);
      con->recv_buffer_ptr= con->recv_buffer;

      recv_size= gearman_connection_read(con, con->recv_buffer + con->recv_buffer_size,
                                         GEARMAN_RECV_BUFFER_SIZE - con->recv_buffer_size,
                                         ret_ptr);
      if (*ret_ptr != GEARMAN_SUCCESS)
        return NULL;

      con->recv_buffer_size+= recv_size;
    }

    if (packet->data_size == 0)
    {
      con->recv_state= GEARMAN_CON_RECV_STATE_NONE;
      break;
    }

    con->recv_data_size= packet->data_size;

    if (!recv_data)
    {
      con->recv_state= GEARMAN_CON_RECV_STATE_READ_DATA;
      break;
    }

    if (packet->gearman->workload_malloc_fn == NULL)
      packet->data= malloc(packet->data_size);
    else
    {
      packet->data= packet->gearman->workload_malloc_fn(packet->data_size,
                                                        (void *)packet->gearman->workload_malloc_context);
    }
    if (packet->data == NULL)
    {
      *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      gearman_connection_close(con);
      return NULL;
    }

    packet->options.free_data= true;
    con->recv_state= GEARMAN_CON_RECV_STATE_READ_DATA;

  case GEARMAN_CON_RECV_STATE_READ_DATA:
    while (con->recv_data_size != 0)
    {
      (void)gearman_connection_recv_data(con,
                                         ((uint8_t *)(packet->data)) +
                                         con->recv_data_offset,
                                         packet->data_size -
                                         con->recv_data_offset, ret_ptr);
      if (*ret_ptr != GEARMAN_SUCCESS)
        return NULL;
    }

    con->recv_state= GEARMAN_CON_RECV_STATE_NONE;
    break;

  default:
    gearman_set_error(con->gearman, "gearman_connection_recv", "unknown state: %u",
                      con->recv_state);
    *ret_ptr= GEARMAN_UNKNOWN_STATE;
    return NULL;
  }

  packet= con->recv_packet;
  con->recv_packet= NULL;

  return packet;
}

size_t gearman_connection_recv_data(gearman_connection_st *con, void *data, size_t data_size,
                                    gearman_return_t *ret_ptr)
{
  size_t recv_size= 0;

  if (con->recv_data_size == 0)
  {
    *ret_ptr= GEARMAN_SUCCESS;
    return 0;
  }

  if ((con->recv_data_size - con->recv_data_offset) < data_size)
    data_size= con->recv_data_size - con->recv_data_offset;

  if (con->recv_buffer_size > 0)
  {
    if (con->recv_buffer_size < data_size)
      recv_size= con->recv_buffer_size;
    else
      recv_size= data_size;

    memcpy(data, con->recv_buffer_ptr, recv_size);
    con->recv_buffer_ptr+= recv_size;
    con->recv_buffer_size-= recv_size;
  }

  if (data_size != recv_size)
  {
    recv_size+= gearman_connection_read(con, ((uint8_t *)data) + recv_size,
                                        data_size - recv_size, ret_ptr);
    con->recv_data_offset+= recv_size;
  }
  else
  {
    con->recv_data_offset+= recv_size;
    *ret_ptr= GEARMAN_SUCCESS;
  }

  if (con->recv_data_size == con->recv_data_offset)
  {
    con->recv_data_size= 0;
    con->recv_data_offset= 0;
    con->recv_state= GEARMAN_CON_RECV_STATE_NONE;
  }

  return recv_size;
}

size_t gearman_connection_read(gearman_connection_st *con, void *data, size_t data_size,
                               gearman_return_t *ret_ptr)
{
  ssize_t read_size;

  while (1)
  {
    read_size= read(con->fd, data, data_size);
    if (read_size == 0)
    {
      if (!(con->options & GEARMAN_CON_IGNORE_LOST_CONNECTION))
      {
        gearman_set_error(con->gearman, "gearman_connection_read",
                          "lost connection to server (EOF)");
      }
      gearman_connection_close(con);
      *ret_ptr= GEARMAN_LOST_CONNECTION;
      return 0;
    }
    else if (read_size == -1)
    {
      if (errno == EAGAIN)
      {
        *ret_ptr= gearman_connection_set_events(con, POLLIN);
        if (*ret_ptr != GEARMAN_SUCCESS)
          return 0;

        if (gearman_state_is_non_blocking(con->gearman))
        {
          *ret_ptr= GEARMAN_IO_WAIT;
          return 0;
        }

        *ret_ptr= gearman_wait(con->gearman);
        if (*ret_ptr != GEARMAN_SUCCESS)
          return 0;

        continue;
      }
      else if (errno == EINTR)
        continue;
      else if (errno == EPIPE || errno == ECONNRESET)
      {
        if (!(con->options & GEARMAN_CON_IGNORE_LOST_CONNECTION))
        {
          gearman_set_error(con->gearman, "gearman_connection_read",
                            "lost connection to server (%d)", errno);
        }
        *ret_ptr= GEARMAN_LOST_CONNECTION;
      }
      else
      {
        gearman_set_error(con->gearman, "gearman_connection_read", "read:%d", errno);
        con->gearman->last_errno= errno;
        *ret_ptr= GEARMAN_ERRNO;
      }

      gearman_connection_close(con);
      return 0;
    }

    break;
  }

  *ret_ptr= GEARMAN_SUCCESS;
  return (size_t)read_size;
}

gearman_return_t gearman_connection_set_events(gearman_connection_st *con, short events)
{
  gearman_return_t ret;

  if ((con->events | events) == con->events)
    return GEARMAN_SUCCESS;

  con->events|= events;

  if (con->gearman->event_watch_fn != NULL)
  {
    ret= con->gearman->event_watch_fn(con, con->events,
                                      (void *)con->gearman->event_watch_context);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_connection_close(con);
      return ret;
    }
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_connection_set_revents(gearman_connection_st *con, short revents)
{
  gearman_return_t ret;

  if (revents != 0)
    con->options|= GEARMAN_CON_READY;

  con->revents= revents;

  /* Remove external POLLOUT watch if we didn't ask for it. Otherwise we spin
    forever until another POLLIN state change. This is much more efficient
    than removing POLLOUT on every state change since some external polling
    mechanisms need to use a system call to change flags (like Linux epoll). */
  if (revents & POLLOUT && !(con->events & POLLOUT) &&
      con->gearman->event_watch_fn != NULL)
  {
    ret= con->gearman->event_watch_fn(con, con->events,
                                      (void *)con->gearman->event_watch_context);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_connection_close(con);
      return ret;
    }
  }

  con->events&= (short)~revents;

  return GEARMAN_SUCCESS;
}

void *gearman_connection_protocol_context(const gearman_connection_st *con)
{
  return (void *)con->protocol_context;
}

void gearman_connection_set_protocol_context(gearman_connection_st *con, const void *context)
{
  con->protocol_context= context;
}

void gearman_connection_set_protocol_context_free_fn(gearman_connection_st *con,
                                                     gearman_connection_protocol_context_free_fn *function)
{
  con->protocol_context_free_fn= function;
}

void gearman_connection_set_packet_pack_fn(gearman_connection_st *con,
                                           gearman_packet_pack_fn *function)
{
  con->packet_pack_fn= function;
}

void gearman_connection_set_packet_unpack_fn(gearman_connection_st *con,
                                             gearman_packet_unpack_fn *function)
{
  con->packet_unpack_fn= function;
}

/*
 * Static Definitions
 */

static gearman_return_t _con_setsockopt(gearman_connection_st *con)
{
  int ret;
  struct linger linger;
  struct timeval waittime;

  ret= 1;
  ret= setsockopt(con->fd, IPPROTO_TCP, TCP_NODELAY, &ret,
                  (socklen_t)sizeof(int));
  if (ret == -1 && errno != EOPNOTSUPP)
  {
    gearman_set_error(con->gearman, "_con_setsockopt",
                      "setsockopt:TCP_NODELAY:%d", errno);
    return GEARMAN_ERRNO;
  }

  linger.l_onoff= 1;
  linger.l_linger= GEARMAN_DEFAULT_SOCKET_TIMEOUT;
  ret= setsockopt(con->fd, SOL_SOCKET, SO_LINGER, &linger,
                  (socklen_t)sizeof(struct linger));
  if (ret == -1)
  {
    gearman_set_error(con->gearman, "_con_setsockopt",
                      "setsockopt:SO_LINGER:%d", errno);
    return GEARMAN_ERRNO;
  }

  waittime.tv_sec= GEARMAN_DEFAULT_SOCKET_TIMEOUT;
  waittime.tv_usec= 0;
  ret= setsockopt(con->fd, SOL_SOCKET, SO_SNDTIMEO, &waittime,
                  (socklen_t)sizeof(struct timeval));
  if (ret == -1 && errno != ENOPROTOOPT)
  {
    gearman_set_error(con->gearman, "_con_setsockopt",
                      "setsockopt:SO_SNDTIMEO:%d", errno);
    return GEARMAN_ERRNO;
  }

  ret= setsockopt(con->fd, SOL_SOCKET, SO_RCVTIMEO, &waittime,
                  (socklen_t)sizeof(struct timeval));
  if (ret == -1 && errno != ENOPROTOOPT)
  {
    gearman_set_error(con->gearman, "_con_setsockopt",
                      "setsockopt:SO_RCVTIMEO:%d", errno);
    return GEARMAN_ERRNO;
  }

  ret= GEARMAN_DEFAULT_SOCKET_SEND_SIZE;
  ret= setsockopt(con->fd, SOL_SOCKET, SO_SNDBUF, &ret, (socklen_t)sizeof(int));
  if (ret == -1)
  {
    gearman_set_error(con->gearman, "_con_setsockopt",
                      "setsockopt:SO_SNDBUF:%d", errno);
    return GEARMAN_ERRNO;
  }

  ret= GEARMAN_DEFAULT_SOCKET_RECV_SIZE;
  ret= setsockopt(con->fd, SOL_SOCKET, SO_RCVBUF, &ret, (socklen_t)sizeof(int));
  if (ret == -1)
  {
    gearman_set_error(con->gearman, "_con_setsockopt",
                      "setsockopt:SO_RCVBUF:%d", errno);
    return GEARMAN_ERRNO;
  }

  ret= fcntl(con->fd, F_GETFL, 0);
  if (ret == -1)
  {
    gearman_set_error(con->gearman, "_con_setsockopt", "fcntl:F_GETFL:%d",
                      errno);
    return GEARMAN_ERRNO;
  }

  ret= fcntl(con->fd, F_SETFL, ret | O_NONBLOCK);
  if (ret == -1)
  {
    gearman_set_error(con->gearman, "_con_setsockopt", "fcntl:F_SETFL:%d",
                      errno);
    return GEARMAN_ERRNO;
  }

  return GEARMAN_SUCCESS;
}
