/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Connection definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearman_con_private Private Connection Functions
 * @ingroup gearman_con
 * @{
 */

/**
 * Set socket options for a connection.
 */
static gearman_return_t _con_setsockopt(gearman_con_st *con);

/**
 * Read data from a connection.
 */
static size_t _con_read(gearman_con_st *con, void *data, size_t data_size,
                        gearman_return_t *ret_ptr);

/** @} */

/*
 * Public definitions
 */

gearman_con_st *gearman_con_add(gearman_st *gearman, gearman_con_st *con,
                                const char *host, in_port_t port)
{
  con= gearman_con_create(gearman, con);
  if (con == NULL)
    return NULL;

  gearman_con_set_host(con, host);
  gearman_con_set_port(con, port);

  return con;
}

gearman_con_st *gearman_con_create(gearman_st *gearman, gearman_con_st *con)
{
  if (con == NULL)
  {
    con= malloc(sizeof(gearman_con_st));
    if (con == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "gearman_con_create", "malloc")
      return NULL;
    }

    memset(con, 0, sizeof(gearman_con_st));
    con->options|= GEARMAN_CON_ALLOCATED;
  }
  else
    memset(con, 0, sizeof(gearman_con_st));

  con->gearman= gearman;

  GEARMAN_LIST_ADD(gearman->con, con,)

  con->fd= -1;
  con->send_buffer_ptr= con->send_buffer;

  return con;
}

gearman_con_st *gearman_con_clone(gearman_st *gearman, gearman_con_st *con,
                                  gearman_con_st *from)
{
  con= gearman_con_create(gearman, con);
  if (con == NULL)
    return NULL;

  con->options|= (from->options & ~GEARMAN_CON_ALLOCATED);
  strcpy(con->host, from->host);
  con->port= from->port;

  return con;
}

gearman_return_t gearman_con_free(gearman_con_st *con)
{
  gearman_return_t ret;

  if (con->fd != -1)
  {
    ret= gearman_con_close(con);
    if (ret != GEARMAN_SUCCESS)
      return ret;
  }

  gearman_con_reset_addrinfo(con);

  GEARMAN_LIST_DEL(con->gearman->con, con,)

  if (con->options & GEARMAN_CON_PACKET_IN_USE)
    gearman_packet_free(&(con->packet));

  if (con->options & GEARMAN_CON_ALLOCATED)
    free(con);

  return GEARMAN_SUCCESS;
}

void gearman_con_set_host(gearman_con_st *con, const char *host)
{
  gearman_con_reset_addrinfo(con);

  strncpy(con->host, host == NULL ? GEARMAN_DEFAULT_TCP_HOST : host,
          NI_MAXHOST);
  con->host[NI_MAXHOST - 1]= 0;
}

void gearman_con_set_port(gearman_con_st *con, in_port_t port)
{
  gearman_con_reset_addrinfo(con);

  con->port= port == 0 ? GEARMAN_DEFAULT_TCP_PORT : port;
}

void gearman_con_set_options(gearman_con_st *con, gearman_con_options_t options,
                             uint32_t data)
{
  if (data)
    con->options|= options;
  else
    con->options&= ~options;
}

gearman_return_t gearman_con_set_fd(gearman_con_st *con, int fd)
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

void *gearman_con_data(gearman_con_st *con)
{
  return con->data;
}
  
void gearman_con_set_data(gearman_con_st *con, void *data)
{
  con->data= data;
}

gearman_return_t gearman_con_connect(gearman_con_st *con)
{
  return gearman_con_flush(con);
}

gearman_return_t gearman_con_close(gearman_con_st *con)
{
  int ret;

  if (con->fd == -1)
    return GEARMAN_SUCCESS;

  if (con->options & GEARMAN_CON_EXTERNAL_FD)
    con->options&= ~GEARMAN_CON_EXTERNAL_FD;
  else
  {
    ret= close(con->fd);
    if (ret == -1)
    {
      GEARMAN_ERROR_SET(con->gearman, "gearman_con_close", "close:%d", errno)
      con->gearman->last_errno= errno;
      return GEARMAN_ERRNO;
    }
  }

  con->state= GEARMAN_CON_STATE_ADDRINFO;
  con->fd= -1;
  con->events= 0;
  con->revents= 0;

  con->send_state= GEARMAN_CON_SEND_STATE_NONE;
  con->send_buffer_ptr= con->send_buffer;
  con->send_buffer_size= 0;
  con->send_data_size= 0;
  con->send_data_offset= 0;

  if (con->recv_packet != NULL)
    gearman_packet_free(con->recv_packet);
  con->recv_buffer_ptr= con->recv_buffer;
  con->recv_buffer_size= 0;

  return GEARMAN_SUCCESS;
}

void gearman_con_reset_addrinfo(gearman_con_st *con)
{
  if (con->addrinfo != NULL)
  {
    freeaddrinfo(con->addrinfo);
    con->addrinfo= NULL;
  }

  con->addrinfo_next= NULL;
}

gearman_return_t gearman_con_send(gearman_con_st *con,
                                  gearman_packet_st *packet, bool flush)
{
  gearman_return_t ret;

  switch (con->send_state)
  {
  case GEARMAN_CON_SEND_STATE_NONE:
    if (!(packet->options & GEARMAN_PACKET_COMPLETE))
    {
      GEARMAN_ERROR_SET(con->gearman, "gearman_con_send", "packet not complete")
      return GEARMAN_INVALID_PACKET;
    }

    /* Flush buffer now if args wont' fit in. */
    if (packet->args_size > (GEARMAN_SEND_BUFFER_SIZE - con->send_buffer_size))
    {
      con->send_state= GEARMAN_CON_SEND_STATE_PRE_FLUSH;

  case GEARMAN_CON_SEND_STATE_PRE_FLUSH:
      ret= gearman_con_flush(con);
      if (ret != GEARMAN_SUCCESS)
        return ret;
    }

    memcpy(con->send_buffer + con->send_buffer_size, packet->args,
           packet->args_size);
    con->send_buffer_size+= packet->args_size;

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
    ret= gearman_con_flush(con);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    con->send_data_size= packet->data_size;

    /* If this is NULL, then gearman_con_send_data function will be used. */
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
    return gearman_con_flush(con);
  }

  if (flush)
  {
    con->send_state= GEARMAN_CON_SEND_STATE_FLUSH;
    return gearman_con_flush(con);
  }

  con->send_state= GEARMAN_CON_SEND_STATE_NONE;
  return GEARMAN_SUCCESS;
}

size_t gearman_con_send_data(gearman_con_st *con, const void *data,
                             size_t data_size, gearman_return_t *ret_ptr)
{
  if (con->send_state != GEARMAN_CON_SEND_STATE_FLUSH_DATA)
  {
    GEARMAN_ERROR_SET(con->gearman, "gearman_con_send_data", "not flushing")
    return GEARMAN_NOT_FLUSHING;
  }

  if (data_size > (con->send_data_size - con->send_data_offset))
  {
    GEARMAN_ERROR_SET(con->gearman, "gearman_con_send_data", "data too large")
    return GEARMAN_DATA_TOO_LARGE;
  }

  con->send_buffer_ptr= (uint8_t *)data;
  con->send_buffer_size= data_size;

  *ret_ptr= gearman_con_flush(con);

  return data_size - con->send_buffer_size;
}

gearman_return_t gearman_con_flush(gearman_con_st *con)
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

      sprintf(port_str, "%u", con->port);

      memset(&ai, 0, sizeof(struct addrinfo));
      ai.ai_flags= (AI_V4MAPPED | AI_ADDRCONFIG);
      ai.ai_family= AF_UNSPEC;
      ai.ai_socktype= SOCK_STREAM;
      ai.ai_protocol= IPPROTO_TCP;

      ret= getaddrinfo(con->host, port_str, &ai, &(con->addrinfo));
      if (ret != 0)
      {
        GEARMAN_ERROR_SET(con->gearman, "gearman_con_flush", "getaddringo:%s",
                          gai_strerror(ret))
        return GEARMAN_GETADDRINFO;
      }

      con->addrinfo_next= con->addrinfo;

    case GEARMAN_CON_STATE_CONNECT:
      if (con->fd != -1)
        (void)gearman_con_close(con);

      if (con->addrinfo_next == NULL)
      {
        con->state= GEARMAN_CON_STATE_ADDRINFO;
        GEARMAN_ERROR_SET(con->gearman, "gearman_con_flush",
                          "could not connect")
        return GEARMAN_COULD_NOT_CONNECT;
      }

      con->fd= socket(con->addrinfo_next->ai_family,
                      con->addrinfo_next->ai_socktype,
                      con->addrinfo_next->ai_protocol);
      if (con->fd == -1)
      {
        con->state= GEARMAN_CON_STATE_ADDRINFO;
        GEARMAN_ERROR_SET(con->gearman, "gearman_con_flush", "socket:%d", errno)
        con->gearman->last_errno= errno;
        return GEARMAN_ERRNO;
      }

      gret= _con_setsockopt(con);
      if (gret != GEARMAN_SUCCESS)
      {
        con->gearman->last_errno= errno;
        (void)gearman_con_close(con);
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

        GEARMAN_ERROR_SET(con->gearman, "gearman_con_flush", "connect:%d",
                          errno)
        con->gearman->last_errno= errno;
        (void)gearman_con_close(con);
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

        gret= gearman_con_set_events(con, POLLOUT);
        if (gret != GEARMAN_SUCCESS)
          return gret;

        if (con->gearman->options & GEARMAN_NON_BLOCKING)
        {
          con->state= GEARMAN_CON_STATE_CONNECTING;
          return GEARMAN_IO_WAIT;
        }

        gret= gearman_con_wait(con->gearman);
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
          GEARMAN_ERROR_SET(con->gearman, "gearman_con_flush", "write:EOF")
          (void)gearman_con_close(con);
          return GEARMAN_EOF;
        }
        else if (write_size == -1)
        {
          if (errno == EAGAIN)
          { 
            gret= gearman_con_set_events(con, POLLOUT);
            if (gret != GEARMAN_SUCCESS)
              return gret;

            if (con->gearman->options & GEARMAN_NON_BLOCKING)
              return GEARMAN_IO_WAIT;

            gret= gearman_con_wait(con->gearman);
            if (gret != GEARMAN_SUCCESS)
              return gret;

            continue;
          } 
          else if (errno == EINTR)
            continue;
      
          GEARMAN_ERROR_SET(con->gearman, "gearman_con_flush", "write:%d",
                            errno)
          con->gearman->last_errno= errno;
          (void)gearman_con_close(con);
          return GEARMAN_ERRNO;
        }

        con->send_buffer_size-= write_size;
        if (con->send_state == GEARMAN_CON_SEND_STATE_FLUSH_DATA)
        {
          con->send_data_offset+= write_size;
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
    }
  }
}

gearman_return_t gearman_con_flush_all(gearman_st *gearman)
{
  gearman_con_st *con;
  gearman_return_t ret;

  for (con= gearman->con_list; con != NULL; con= con->next)
  {
    if (con->events & POLLOUT)
      continue;

    ret= gearman_con_flush(con);
    if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
      return ret;
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_con_send_all(gearman_st *gearman,
                                      gearman_packet_st *packet)
{
  gearman_return_t ret;
  gearman_con_st *con;
  gearman_options_t options= gearman->options;

  gearman->options|= GEARMAN_NON_BLOCKING;

  if (gearman->sending == 0)
  {
    for (con= gearman->con_list; con != NULL; con= con->next)
    { 
      ret= gearman_con_send(con, packet, true);
      if (ret != GEARMAN_SUCCESS)
      { 
        if (ret != GEARMAN_IO_WAIT)
        {
          gearman->options= options;
          return ret;
        }

        gearman->sending++;
        break;
      }
    }
  }

  while (gearman->sending != 0)
  {
    while ((con= gearman_con_ready(gearman)) != NULL)
    { 
      ret= gearman_con_send(con, packet, true);
      if (ret != GEARMAN_SUCCESS)
      { 
        if (ret != GEARMAN_IO_WAIT)
        {
          gearman->options= options;
          return ret;
        }

        continue;
      }

      gearman->sending--;
    }

    if (gearman->sending == 0)
      break;

    if (options & GEARMAN_NON_BLOCKING)
    {
      gearman->options= options;
      return GEARMAN_IO_WAIT;
    }

    ret= gearman_con_wait(gearman);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman->options= options;
      return ret;
    }
  }

  gearman->options= options;
  return GEARMAN_SUCCESS;
}

gearman_packet_st *gearman_con_recv(gearman_con_st *con,
                                    gearman_packet_st *packet,
                                    gearman_return_t *ret_ptr, bool recv_data)
{
  size_t recv_size;

  switch (con->recv_state)
  {
  case GEARMAN_CON_RECV_STATE_NONE:
    if (con->state != GEARMAN_CON_STATE_CONNECTED)
    {
      GEARMAN_ERROR_SET(con->gearman, "gearman_con_recv", "not connected")
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
        recv_size= gearman_packet_parse(con->recv_packet, con->recv_buffer_ptr,
                                        con->recv_buffer_size, ret_ptr);
        con->recv_buffer_ptr+= recv_size;
        con->recv_buffer_size-= recv_size;
        if (*ret_ptr == GEARMAN_SUCCESS)
          break;
        else if (*ret_ptr != GEARMAN_IO_WAIT)
        {
          (void)gearman_con_close(con);
          return NULL;
        }
      }

      /* Shift buffer contents if needed. */
      if (con->recv_buffer_size > 0)
        memmove(con->recv_buffer, con->recv_buffer_ptr, con->recv_buffer_size);
      con->recv_buffer_ptr= con->recv_buffer;

      recv_size= _con_read(con, con->recv_buffer + con->recv_buffer_size,
                           GEARMAN_RECV_BUFFER_SIZE - con->recv_buffer_size,
                           ret_ptr);
      if (*ret_ptr != GEARMAN_SUCCESS)
        return NULL;

      con->recv_buffer_size+= recv_size;
    }

    if (packet->data_size == 0)
      break;

    con->recv_data_size= packet->data_size;

    if (!recv_data)
      break;

    if (packet->gearman->workload_malloc == NULL)
      packet->data= malloc(packet->data_size);
    else
    {
      packet->data= packet->gearman->workload_malloc(packet->data_size,
                                (void *)(packet->gearman->workload_malloc_arg));
    }
    if (packet->data == NULL)
    {
      *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      (void)gearman_con_close(con);
      return NULL;
    }

    packet->options|= GEARMAN_PACKET_FREE_DATA;
    con->recv_state= GEARMAN_CON_RECV_STATE_READ_DATA;

  case GEARMAN_CON_RECV_STATE_READ_DATA:
    while (con->recv_data_size != 0)
    {
      recv_size= gearman_con_recv_data(con,
                                       ((uint8_t *)(packet->data)) +
                                       con->recv_data_offset,
                                       packet->data_size -
                                       con->recv_data_offset, ret_ptr);
      if (*ret_ptr != GEARMAN_SUCCESS)
        return NULL;
    }

    break;
  }

  con->recv_state= GEARMAN_CON_RECV_STATE_NONE;
  packet= con->recv_packet;
  con->recv_packet= NULL;

  return packet;
}

size_t gearman_con_recv_data(gearman_con_st *con, void *data, size_t data_size,
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
    recv_size+= _con_read(con, ((uint8_t *)data) + recv_size,
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
  }

  return recv_size;
}

gearman_return_t gearman_con_wait(gearman_st *gearman)
{
  gearman_con_st *con;
  struct pollfd *pfds;
  int x; 
  int ret;

  if (gearman->pfds_size < gearman->con_count)
  {
    pfds= realloc(gearman->pfds, gearman->con_count * sizeof(struct pollfd));
    if (pfds == NULL)
    { 
      GEARMAN_ERROR_SET(gearman, "gearman_con_wait", "realloc")
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    gearman->pfds= pfds;
    gearman->pfds_size= gearman->con_count;
  }  
  else
    pfds= gearman->pfds;

  x= 0;
  for (con= gearman->con_list; con != NULL; con= con->next)
  {
    if (con->events == 0)
      continue;

    pfds[x].fd= con->fd;
    pfds[x].events= con->events;
    pfds[x].revents= 0;
    x++;
  }
  
  if (x == 0)
  {
    GEARMAN_ERROR_SET(gearman, "gearman_con_wait", "no active file descriptors")
    return GEARMAN_NO_ACTIVE_FDS;
  }

  ret= poll(pfds, x, -1);
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(gearman, "gearman_con_wait", "poll:%d", errno)
    gearman->last_errno= errno;
    return GEARMAN_ERRNO;
  }
  
  x= 0;
  for (con= gearman->con_list; con != NULL; con= con->next)
  {
    if (pfds[x].events == 0)
      continue;

    gearman_con_set_revents(con, pfds[x].revents);
    x++;
  }
  
  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_con_set_events(gearman_con_st *con, short events)
{
  gearman_return_t ret;

  con->events|= events;

  if (con->gearman->event_watch != NULL)
  {
    ret= (con->gearman->event_watch)(con, events,
                                     con->gearman->event_watch_arg);
    if (ret != GEARMAN_SUCCESS)
    {
      (void)gearman_con_close(con);
      return ret;
    }
  }

  return GEARMAN_SUCCESS;
}

void gearman_con_set_revents(gearman_con_st *con, short revents)
{
  if (revents != 0)
    con->options|= GEARMAN_CON_READY;

  con->revents= revents;
  con->events&= ~revents;
}

gearman_con_st *gearman_con_ready(gearman_st *gearman)
{
  gearman_con_st *con;

  /* We can't keep state between calls since connections may be removed during
     processing. If this list ever gets big, we may want something faster. */

  for (con= gearman->con_list; con != NULL; con= con->next)
  {
    if (con->options & GEARMAN_CON_READY)
    {
      con->options&= ~GEARMAN_CON_READY;
      return con;
    }
  }
  
  return NULL;
}

gearman_return_t gearman_con_echo(gearman_st *gearman, const void *workload,
                                  size_t workload_size)
{
  gearman_con_st *con;
  gearman_options_t options= gearman->options;
  gearman_packet_st packet;
  gearman_return_t ret;

  ret= gearman_packet_add(gearman, &packet, GEARMAN_MAGIC_REQUEST,
                          GEARMAN_COMMAND_ECHO_REQ, workload, workload_size,
                          NULL);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  gearman->options&= ~GEARMAN_NON_BLOCKING;

  for (con= gearman->con_list; con != NULL; con= con->next)
  {
    ret= gearman_con_send(con, &packet, true);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_packet_free(&packet);
      gearman->options= options;
      return ret;
    }

    (void)gearman_con_recv(con, &(con->packet), &ret, true);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_packet_free(&packet);
      gearman->options= options;
      return ret;
    }

    if (con->packet.data_size != workload_size ||
        memcmp(workload, con->packet.data, workload_size))
    {
      gearman_packet_free(&(con->packet));
      gearman_packet_free(&packet);
      gearman->options= options;
      GEARMAN_ERROR_SET(gearman, "gearman_con_echo", "corruption during echo")
      return GEARMAN_ECHO_DATA_CORRUPTION;
    }

    gearman_packet_free(&(con->packet));
  }

  gearman_packet_free(&packet);
  gearman->options= options;
  return GEARMAN_SUCCESS;
}

/*
 * Private definitions
 */

static gearman_return_t _con_setsockopt(gearman_con_st *con)
{
  int ret;
  struct linger linger;
  struct timeval waittime;

  ret= 1;
  ret= setsockopt(con->fd, IPPROTO_TCP, TCP_NODELAY, &ret,
                  (socklen_t)sizeof(int));
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt",
                      "setsockopt:TCP_NODELAY:%d", errno)
    return GEARMAN_ERRNO;
  }

  linger.l_onoff= 1;
  linger.l_linger= GEARMAN_DEFAULT_SOCKET_TIMEOUT;
  ret= setsockopt(con->fd, SOL_SOCKET, SO_LINGER, &linger,
                  (socklen_t)sizeof(struct linger));
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt",
                      "setsockopt:SO_LINGER:%d", errno)
    return GEARMAN_ERRNO;
  }

  waittime.tv_sec= GEARMAN_DEFAULT_SOCKET_TIMEOUT;
  waittime.tv_usec= 0;
  ret= setsockopt(con->fd, SOL_SOCKET, SO_SNDTIMEO, &waittime,
                  (socklen_t)sizeof(struct timeval));
  if (ret == -1 && errno != ENOPROTOOPT)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt",
                      "setsockopt:SO_SNDTIMEO:%d", errno)
    return GEARMAN_ERRNO;
  }

  ret= setsockopt(con->fd, SOL_SOCKET, SO_RCVTIMEO, &waittime,
                  (socklen_t)sizeof(struct timeval));
  if (ret == -1 && errno != ENOPROTOOPT)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt",
                      "setsockopt:SO_RCVTIMEO:%d", errno)
    return GEARMAN_ERRNO;
  }

  ret= GEARMAN_DEFAULT_SOCKET_SEND_SIZE;
  ret= setsockopt(con->fd, SOL_SOCKET, SO_SNDBUF, &ret, (socklen_t)sizeof(int));
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt",
                      "setsockopt:SO_SNDBUF:%d", errno)
    return GEARMAN_ERRNO;
  }

  ret= GEARMAN_DEFAULT_SOCKET_RECV_SIZE;
  ret= setsockopt(con->fd, SOL_SOCKET, SO_RCVBUF, &ret, (socklen_t)sizeof(int));
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt",
                      "setsockopt:SO_RCVBUF:%d", errno)
    return GEARMAN_ERRNO;
  }

#ifdef FIONBIO
  ret= 1;
  ret= ioctl(con->fd, FIONBIO, &ret);
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt", "ioctl:FIONBIO:%d",
                      errno)
    return GEARMAN_ERRNO;
  }
#else /* !FIONBIO */
  ret= fcntl(con->fd, F_GETFL, 0);
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt", "fcntl:F_GETFL:%d",
                      errno)
    return GEARMAN_ERRNO;
  }

  ret= fcntl(con->fd, F_SETFL, ret | O_NONBLOCK);
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt", "fcntl:F_SETFL:%d",
                      errno)
    return GEARMAN_ERRNO;
  }
#endif /* FIONBIO */

  return GEARMAN_SUCCESS;
}

static size_t _con_read(gearman_con_st *con, void *data, size_t data_size,
                        gearman_return_t *ret_ptr)
{
  ssize_t read_size;

  while (1)
  {
    read_size= read(con->fd, data, data_size);
    if (read_size == 0)
    {
      GEARMAN_ERROR_SET(con->gearman, "_con_read", "read:EOF")
      (void)gearman_con_close(con);
      *ret_ptr= GEARMAN_EOF;
      return 0;
    }
    else if (read_size == -1)
    {
      if (errno == EAGAIN)
      {
        *ret_ptr= gearman_con_set_events(con, POLLIN);
        if (*ret_ptr != GEARMAN_SUCCESS)
          return 0;

        if (con->gearman->options & GEARMAN_NON_BLOCKING)
        {
          *ret_ptr= GEARMAN_IO_WAIT;
          return 0;
        }

        *ret_ptr= gearman_con_wait(con->gearman);
        if (*ret_ptr != GEARMAN_SUCCESS)
          return 0;

        continue;
      }
      else if (errno == EINTR)
        continue;

      GEARMAN_ERROR_SET(con->gearman, "_con_read", "read:%d", errno)
      con->gearman->last_errno= errno;
      (void)gearman_con_close(con);
      *ret_ptr= GEARMAN_ERRNO;
      return 0;
    }

    break;
  }

  *ret_ptr= GEARMAN_SUCCESS;
  return read_size;
}
