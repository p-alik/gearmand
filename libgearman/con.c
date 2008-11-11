/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
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

/* Parse packet from input data. */
static gearman_return _con_parse_packet(gearman_con_st *con);

/* Set socket options for a connection. */
static gearman_return _con_setsockopt(gearman_con_st *con);

/* Common usage, add a connection structure. */
gearman_con_st *gearman_con_add(gearman_st *gearman, gearman_con_st *con,
                                char *host, in_port_t port)
{
  con= gearman_con_create(gearman, con);
  if (con == NULL)
    return NULL;

  gearman_con_set_host(con, host);
  gearman_con_set_port(con, port);

  return con;
}

/* Initialize a connection structure. */
gearman_con_st *gearman_con_create(gearman_st *gearman, gearman_con_st *con)
{
  if (con == NULL)
  {
    con= malloc(sizeof(gearman_con_st));
    if (con == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "gearman_con_create:malloc");
      return NULL;
    }

    memset(con, 0, sizeof(gearman_con_st));
    con->options|= GEARMAN_CON_ALLOCATED;
  }
  else
    memset(con, 0, sizeof(gearman_con_st));

  con->gearman= gearman;

  if (gearman->con_list)
    gearman->con_list->prev= con;
  con->next= gearman->con_list;
  gearman->con_list= con;
  gearman->con_count++;

  con->fd= -1;

  return con;
}

/* Clone a connection structure. */
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

/* Free a connection structure. */
gearman_return gearman_con_free(gearman_con_st *con)
{
  gearman_return ret;

  if (con->fd != -1)
  {
    ret= gearman_con_close(con);
    if (ret != GEARMAN_SUCCESS)
      return ret;
  }

  gearman_con_reset_addrinfo(con);

  if (con->gearman->con_list == con)
    con->gearman->con_list= con->next;
  if (con->prev)
    con->prev->next= con->next;
  if (con->next)
    con->next->prev= con->prev;
  con->gearman->con_count--;

  if (con->options & GEARMAN_CON_ALLOCATED)
    free(con);

  return GEARMAN_SUCCESS;
}

/* Set options for a connection. */
void gearman_con_set_host(gearman_con_st *con, char *host)
{
  gearman_con_reset_addrinfo(con);

  strncpy(con->host, host == NULL ? GEARMAN_DEFAULT_TCP_HOST : host,
          NI_MAXHOST);
}

void gearman_con_set_port(gearman_con_st *con, in_port_t port)
{
  gearman_con_reset_addrinfo(con);

  con->port= port == 0 ? GEARMAN_DEFAULT_TCP_PORT : port;
}

void gearman_con_set_options(gearman_con_st *con, gearman_con_options options,
                             uint32_t data)
{
  if (data)
    con->options|= options;
  else
    con->options&= ~options;
}

/* Set and get application data pointer for a connection. */
void gearman_con_set_data(gearman_con_st *con, void *data)
{
  con->data= data;
}

void *gearman_con_get_data(gearman_con_st *con)
{
  return con->data;
}

/* Set connection to an already open file description. */
void gearman_con_set_fd(gearman_con_st *con, int fd)
{
  con->fd= fd;
}

/* Connect to server. */
gearman_return gearman_con_connect(gearman_con_st *con)
{
  return gearman_con_loop(con);
}

/* Close a connection. */
gearman_return gearman_con_close(gearman_con_st *con)
{
  int ret;

  if (con->fd == -1)
    return GEARMAN_SUCCESS;;

  ret= close(con->fd);
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "gearman_con_close:close:%d", errno);
    con->gearman->last_errno= errno;
    return GEARMAN_ERRNO;
  }

  con->state= GEARMAN_CON_STATE_ADDRINFO;
  con->fd= -1;
  con->events= 0;
  con->revents= 0;

  return GEARMAN_SUCCESS;;
}

/* Clear address info, freeing structs if needed. */
void gearman_con_reset_addrinfo(gearman_con_st *con)
{
  if (con->addrinfo != NULL)
  {
    freeaddrinfo(con->addrinfo);
    con->addrinfo= NULL;
  }

  con->addrinfo_next= NULL;
}

/* Send packet to a connection. */
gearman_return gearman_con_send(gearman_con_st *con, gearman_packet_st *packet)
{
  if (!(packet->options & GEARMAN_PACKET_PACKED))
  {
    GEARMAN_ERROR_SET(con->gearman, "gearman_con_send:packet not packed");
    return GEARMAN_INVALID_PACKET;
  }

  if (con->send_packet == NULL)
  {
    con->send_packet= packet;
    con->send_packet_ptr= packet->data;
    con->send_packet_size= packet->data_size;
  }
  else if(con->send_packet != packet)
  {
    GEARMAN_ERROR_SET(con->gearman, "gearman_con_send:send in progress");
    return GEARMAN_SEND_IN_PROGRESS;
  }

  return gearman_con_loop(con);
}

/* Send packet to all connections. */
gearman_return gearman_con_send_all(gearman_st *gearman,
                                    gearman_packet_st *packet)
{
  gearman_return ret;
  gearman_con_st *con;
  gearman_options old_options;

  if (gearman->sending == 0)
  {
    old_options= gearman->options;
    gearman->options&= ~GEARMAN_NON_BLOCKING;

    for (con= gearman->con_list; con != NULL; con= con->next)
    { 
      ret= gearman_con_send(con, packet);
      if (ret != GEARMAN_SUCCESS)
      { 
        if (ret != GEARMAN_IO_WAIT)
        {
          gearman->options= old_options;
          return ret;
        }

        gearman->sending++;
        break;
      }
    }

    gearman->options= old_options;
  }

  while (gearman->sending != 0)
  {
    while (1)
    { 
      con= gearman_io_ready(gearman);
      if (con == NULL)
        break;

      ret= gearman_con_send(con, packet);
      if (ret != GEARMAN_SUCCESS)
      { 
        if (ret != GEARMAN_IO_WAIT)
          return ret;

        continue;
      }

      gearman->sending--;
    }

    if (gearman->options & GEARMAN_NON_BLOCKING)
      return GEARMAN_IO_WAIT;

    ret= gearman_io_wait(gearman, false);
    if (ret != GEARMAN_IO_WAIT)
      return ret;
  }

  return GEARMAN_SUCCESS;
}

/* Receive packet from a connection. */
gearman_packet_st *gearman_con_recv(gearman_con_st *con,
                                    gearman_packet_st *packet,
                                    gearman_return *ret_ptr)
{
  ssize_t read_size;

  if (con->state != GEARMAN_CON_STATE_IDLE)
  {
    GEARMAN_ERROR_SET(con->gearman, "gearman_con_recv:not connected");
    *ret_ptr= GEARMAN_NOT_CONNECTED;
    return NULL;
  }

  if (con->recv_packet == NULL)
  {
    con->recv_packet= gearman_packet_create(con->gearman, packet);
    if (con->recv_packet == NULL)
    {
      *ret_ptr= GEARMAN_MEMORY_ALLOCATION_FAILURE;
      return NULL;
    }
  }
  else if(packet != NULL && con->recv_packet != packet)
  {
    GEARMAN_ERROR_SET(con->gearman, "gearman_con_recv:recv in progress");
    *ret_ptr= GEARMAN_RECV_IN_PROGRESS;
    return NULL;
  }

  while (1)
  {
    if (con->recv_buffer_size > 0)
    {
      *ret_ptr= _con_parse_packet(con);
      if (*ret_ptr == GEARMAN_SUCCESS)
        break;
      else if (*ret_ptr != GEARMAN_IO_WAIT)
      {
        gearman_packet_free(con->recv_packet);
        (void)gearman_con_close(con);
        return NULL;
      }
    }

    /* Shift buffer contents if needed. */
    if (con->recv_buffer_size == 0)
      con->recv_buffer_ptr= con->recv_buffer;
    else if ((con->recv_buffer_ptr - con->recv_buffer) >
             (GEARMAN_READ_BUFFER_SIZE / 2))
    {
      memmove(con->recv_buffer, con->recv_buffer_ptr, con->recv_buffer_size);
      con->recv_buffer_ptr= con->recv_buffer;
    }

    read_size= read(con->fd, con->recv_buffer_ptr + con->recv_buffer_size,
                    GEARMAN_READ_BUFFER_SIZE -
                    ((con->recv_buffer_ptr - con->recv_buffer) +
                     con->recv_buffer_size));
    if (read_size == 0)
    {
      gearman_packet_free(con->recv_packet);
      GEARMAN_ERROR_SET(con->gearman, "gearman_con_recv:read:EOF");
      (void)gearman_con_close(con);
      *ret_ptr= GEARMAN_EOF;
      return NULL;
    }
    else if (read_size == -1)
    {
      if (errno == EAGAIN)
      {
        con->events= POLLIN;

        if (con->gearman->options & GEARMAN_NON_BLOCKING)
        {
          *ret_ptr= GEARMAN_IO_WAIT;
          return NULL;
        }

        *ret_ptr= gearman_io_wait(con->gearman, false);
        if (*ret_ptr != GEARMAN_SUCCESS)
          return NULL;

        continue;
      }
      else if (errno == EINTR)
        continue;

      gearman_packet_free(con->recv_packet);
      GEARMAN_ERROR_SET(con->gearman, "gearman_con_recv:read:%d", errno);
      con->gearman->last_errno= errno;
      (void)gearman_con_close(con);
      *ret_ptr= GEARMAN_ERRNO;
      return NULL;
    }

    con->recv_buffer_size+= read_size;
  }

  packet= con->recv_packet;
  con->recv_packet= NULL;

  return packet;
}

/* State loop for gearman_con_st. */
gearman_return gearman_con_loop(gearman_con_st *con)
{
  char port_str[NI_MAXSERV];
  struct addrinfo ai;
  int ret;
  ssize_t write_size;
  gearman_return gret;

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
        GEARMAN_ERROR_SET(con->gearman, "gearman_con_loop:getaddringo:%s",
                          gai_strerror(ret));
        return GEARMAN_GETADDRINFO;
      }

      con->addrinfo_next= con->addrinfo;

    case GEARMAN_CON_STATE_CONNECT:
      if (con->fd != -1)
        (void)gearman_con_close(con);

      if (con->addrinfo_next == NULL)
      {
        con->state= GEARMAN_CON_STATE_ADDRINFO;
        GEARMAN_ERROR_SET(con->gearman, "gearman_con_loop:could not connect");
        return GEARMAN_COULD_NOT_CONNECT;
      }

      con->fd= socket(con->addrinfo_next->ai_family,
                      con->addrinfo_next->ai_socktype,
                      con->addrinfo_next->ai_protocol);
      if (con->fd == -1)
      {
        con->state= GEARMAN_CON_STATE_ADDRINFO;
        GEARMAN_ERROR_SET(con->gearman, "gearman_con_loop:socket:%d", errno);
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
          con->state= GEARMAN_CON_STATE_IDLE;
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

        GEARMAN_ERROR_SET(con->gearman, "gearman_con_loop:connect:%d", errno);
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
          con->state= GEARMAN_CON_STATE_IDLE;
          break;
        }
        else if (con->revents & (POLLERR | POLLHUP | POLLNVAL))
        {
          con->state= GEARMAN_CON_STATE_CONNECT;
          con->addrinfo_next= con->addrinfo_next->ai_next;
          break;
        }

        con->events= POLLOUT;

        if (con->gearman->options & GEARMAN_NON_BLOCKING)
        {
          con->state= GEARMAN_CON_STATE_CONNECTING;
          return GEARMAN_IO_WAIT;
        }

        gret= gearman_io_wait(con->gearman, false);
        if (gret != GEARMAN_SUCCESS)
          return gret;
      }

      if (con->state != GEARMAN_CON_STATE_IDLE)
        break;

    case GEARMAN_CON_STATE_IDLE:
      while (con->send_packet_size != 0)
      {
        write_size= write(con->fd, con->send_packet_ptr, con->send_packet_size);
        if (write_size == 0)
        {
          GEARMAN_ERROR_SET(con->gearman, "gearman_con_loop:write:EOF");
          (void)gearman_con_close(con);
          return GEARMAN_EOF;
        }
        else if (write_size == -1)
        {
          if (errno == EAGAIN)
          { 
            con->events= POLLOUT;

            if (con->gearman->options & GEARMAN_NON_BLOCKING)
              return GEARMAN_IO_WAIT;

            gret= gearman_io_wait(con->gearman, false);
            if (gret != GEARMAN_SUCCESS)
              return gret;

            continue;
          } 
          else if (errno == EINTR)
            continue;
      
          GEARMAN_ERROR_SET(con->gearman, "gearman_con_loop:write:%d", errno);
          con->gearman->last_errno= errno;
          (void)gearman_con_close(con);
          return GEARMAN_ERRNO;
        }

        con->send_packet_size-= write_size;
        if (con->send_packet_size == 0)
        {
          con->send_packet= NULL;
          break;
        }

        con->send_packet_ptr+= write_size;
      }

      return GEARMAN_SUCCESS;
    }
  }

  return GEARMAN_SUCCESS;
}

/* Parse packet from input data. */
static gearman_return _con_parse_packet(gearman_con_st *con)
{
  uint32_t data_size;
  size_t data_read;
  gearman_return ret;

  if (con->recv_packet_size == 0)
  {
    if (con->recv_buffer_size < 12)
      return GEARMAN_IO_WAIT;

    memcpy(&data_size, con->recv_buffer_ptr + 8, 4);
    con->recv_packet_size= ntohl(data_size) + 12;
  }

  if ((size_t)(con->recv_buffer_size) < con->recv_packet_size)
    data_read= con->recv_buffer_size;
  else
    data_read= con->recv_packet_size;

  ret= gearman_packet_add_arg_data(con->recv_packet, con->recv_buffer_ptr,
                                   data_read);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  con->recv_packet_size-= data_read;
  con->recv_buffer_ptr+= data_read;
  con->recv_buffer_size-= data_read;

  if (con->recv_packet_size != 0)
    return GEARMAN_IO_WAIT;

  return gearman_packet_unpack(con->recv_packet);
}

/* Set socket options for a connection. */
static gearman_return _con_setsockopt(gearman_con_st *con)
{
  int ret;
  struct linger linger;
  struct timeval waittime;

  ret= 1;
  ret= setsockopt(con->fd, IPPROTO_TCP, TCP_NODELAY, &ret,
                  (socklen_t)sizeof(int));
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt:setsockopt:TCP_NODELAY:%d",
                      errno);
    return GEARMAN_ERRNO;
  }

  linger.l_onoff= 1;
  linger.l_linger= GEARMAN_DEFAULT_SOCKET_TIMEOUT;
  ret= setsockopt(con->fd, SOL_SOCKET, SO_LINGER, &linger,
                  (socklen_t)sizeof(struct linger));
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt:setsockopt:SO_LINGER:%d",
                      errno);
    return GEARMAN_ERRNO;
  }

  waittime.tv_sec= GEARMAN_DEFAULT_SOCKET_TIMEOUT;
  waittime.tv_usec= 0;
  ret= setsockopt(con->fd, SOL_SOCKET, SO_SNDTIMEO, &waittime,
                  (socklen_t)sizeof(struct timeval));
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt:setsockopt:SO_SNDTIMEO:%d",
                      errno);
    return GEARMAN_ERRNO;
  }

  ret= setsockopt(con->fd, SOL_SOCKET, SO_RCVTIMEO, &waittime,
                  (socklen_t)sizeof(struct timeval));
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt:setsockopt:SO_RCVTIMEO:%d",
                      errno);
    return GEARMAN_ERRNO;
  }

  ret= GEARMAN_DEFAULT_SOCKET_SEND_SIZE;
  ret= setsockopt(con->fd, SOL_SOCKET, SO_SNDBUF, &ret, (socklen_t)sizeof(int));
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt:setsockopt:SO_SNDBUF:%d",
                      errno);
    return GEARMAN_ERRNO;
  }

  ret= GEARMAN_DEFAULT_SOCKET_RECV_SIZE;
  ret= setsockopt(con->fd, SOL_SOCKET, SO_RCVBUF, &ret, (socklen_t)sizeof(int));
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt:setsockopt:SO_RCVBUF:%d",
                      errno);
    return GEARMAN_ERRNO;
  }

#ifdef FIONBIO
  ret= 1;
  ret= ioctl(con->fd, FIONBIO, &ret);
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt:ioctl:FIONBIO:%d", errno);
    return GEARMAN_ERRNO;
  }
#else /* !FIONBIO */
  ret= fcntl(con->fd, F_GETFL, 0);
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt:fcntl:F_GETFL:%d", errno);
    return GEARMAN_ERRNO;
  }

  ret= fcntl(con->fd, F_SETFL, ret | O_NONBLOCK);
  if (ret == -1)
  {
    GEARMAN_ERROR_SET(con->gearman, "_con_setsockopt:fcntl:F_SETFL:%d", errno);
    return GEARMAN_ERRNO;
  }
#endif /* FIONBIO */

  return GEARMAN_SUCCESS;
}
