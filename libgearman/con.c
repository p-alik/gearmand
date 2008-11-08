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
      return NULL;

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
#if 0
  if (con->state_current == 0)
  {
    GEARMAN_STATE_PUSH(con, gearman_state_addrinfo);
  }

  return gearman_state_loop(con);
#endif
  con= NULL;
  return GEARMAN_SUCCESS;
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
  con->packet= packet;

  return gearman_con_loop(con);
}

/* Receive packet from a connection. */
gearman_packet_st *gearman_con_recv(gearman_con_st *con,
                                    gearman_packet_st *packet,
                                    gearman_return *ret)
{
  if (con->packet == NULL)
  {
    con->packet= packet;
    con->state= GEARMAN_CON_STATE_RECV;
  }

  *ret= gearman_con_loop(con);
  if (*ret != GEARMAN_SUCCESS)
  {
    if (*ret != GEARMAN_IO_WAIT && con->packet != NULL)
      gearman_packet_free(con->packet);

    return NULL;
  }

  packet= con->packet;
  con->packet= NULL;
  con->state= GEARMAN_CON_STATE_IDLE;

  return packet;
}

/* State loop for gearman_con_st. */
gearman_return gearman_con_loop(gearman_con_st *con)
{
  char port_str[NI_MAXSERV];
  struct addrinfo ai;
  int ret;
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
      con->state= GEARMAN_CON_STATE_CONNECT;

    case GEARMAN_CON_STATE_CONNECT:
      if (con->fd != -1)
        (void)gearman_con_close(con);

      if (con->addrinfo_next == NULL)
        return GEARMAN_ERRNO;

      con->fd= socket(con->addrinfo_next->ai_family,
                      con->addrinfo_next->ai_socktype,
                      con->addrinfo_next->ai_protocol);
      if (con->fd == -1)
      {
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

        GEARMAN_ERROR_SET(con->gearman, "gearman_con_loop:connect:%d", errno);
        con->gearman->last_errno= errno;

        if (errno == ECONNREFUSED || errno == ENETUNREACH || errno == ETIMEDOUT)
        {
          con->addrinfo_next= con->addrinfo_next->ai_next;
          break;
        }

        (void)gearman_con_close(con);
        return GEARMAN_ERRNO;
      }

      con->state= GEARMAN_CON_STATE_IDLE;
      break;

    case GEARMAN_CON_STATE_CONNECTING:
      while (1)
      {
        if (con->revents & POLLOUT)
          break;
        else if (con->revents & (POLLERR | POLLHUP | POLLNVAL))
        {
          GEARMAN_ERROR_SET(con->gearman, "gearman_con_loop:poll error");
          con->gearman->last_errno= 0;
          con->state= GEARMAN_CON_STATE_CONNECT;
          con->addrinfo_next= con->addrinfo_next->ai_next;
          break;
        }

        con->events= POLLOUT;

        if (con->gearman->options & GEARMAN_NON_BLOCKING)
          return GEARMAN_IO_WAIT;

        gret= gearman_io_wait(con->gearman);
        if (gret != GEARMAN_SUCCESS)
          return gret;
      }

      con->state= GEARMAN_CON_STATE_SEND;

    case GEARMAN_CON_STATE_IDLE:
      if (con->packet != NULL)
      {
        con->state= GEARMAN_CON_STATE_SEND;
        con->buffer_ptr= con->packet->data;
        con->buffer_len= con->packet->length;
        break;
      }

      return GEARMAN_SUCCESS;

    case GEARMAN_CON_STATE_SEND:
      while (con->buffer_len != 0)
      {
        ret= write(con->fd, con->buffer_ptr, con->buffer_len);
        if (ret == 0)
        { 
          GEARMAN_ERROR_SET(con->gearman, "gearman_con_loop:write:end of file");
          (void)gearman_con_close(con);
          return GEARMAN_EOF;
        }
        else if (ret == -1)
        { 
          if (errno == EAGAIN)
          { 
            con->events= POLLOUT;

            if (con->gearman->options & GEARMAN_NON_BLOCKING)
              return GEARMAN_IO_WAIT;

            gret= gearman_io_wait(con->gearman);
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

        con->buffer_len-= ret;
        if (con->buffer_len == 0)
          break;

        con->buffer_ptr+= ret;
      }

      con->packet= NULL;
      con->state= GEARMAN_CON_STATE_IDLE;
      break;

    case GEARMAN_CON_STATE_RECV:
      while (1)
      {
        if (con->buffer_len > 0)
        {
          gret= _con_parse_packet(con);
          if (gret != GEARMAN_IO_WAIT)
            return gret;
        }

        if (con->buffer_len == 0)
          con->buffer_ptr= con->buffer;
        else if ((con->buffer_ptr - con->buffer) >
                 (GEARMAN_MAX_BUFFER_LENGTH / 2))
        {
          memmove(con->buffer, con->buffer_ptr, con->buffer_len);
          con->buffer_ptr= con->buffer;
        }

        ret= read(con->fd, con->buffer_ptr + con->buffer_len,
                  GEARMAN_MAX_BUFFER_LENGTH -
                  ((con->buffer_ptr - con->buffer) + con->buffer_len));
        if (ret == 0)
        {
          GEARMAN_ERROR_SET(con->gearman, "gearman_con_loop:read:end of file");
          (void)gearman_con_close(con);
          return GEARMAN_EOF;
        }
        else if (ret == -1)
        {
          if (errno == EAGAIN)
          {
            con->events= POLLIN;

            if (con->gearman->options & GEARMAN_NON_BLOCKING)
              return GEARMAN_IO_WAIT;

            gret= gearman_io_wait(con->gearman);
            if (gret != GEARMAN_SUCCESS)
              return gret;

            continue;
          }
          else if (errno == EINTR)
            continue;

          GEARMAN_ERROR_SET(con->gearman, "gearman_con_loop:read:%d", errno);
          con->gearman->last_errno= errno;
          (void)gearman_con_close(con);
          return GEARMAN_ERRNO;
        }

        con->buffer_len+= ret;
      }
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

  if (con->packet_size == 0)
  {
    if (con->buffer_len < 12)
      return GEARMAN_IO_WAIT;

    memcpy(&data_size, con->buffer_ptr + 8, 4);
    con->packet_size= ntohl(data_size) + 12;

    con->packet= gearman_packet_create(con->gearman, con->packet);
    if (con->packet == NULL)
      return GEARMAN_ERRNO;
  }

  if ((size_t)(con->buffer_len) < con->packet_size)
    data_read= con->buffer_len;
  else
    data_read= con->packet_size;

  ret= gearman_packet_arg_add_data(con->packet, con->buffer_ptr, data_read);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  con->packet_size-= data_read;
  con->buffer_ptr+= data_read;
  con->buffer_len-= data_read;

  if (con->packet_size != 0)
    return GEARMAN_IO_WAIT;

  return gearman_packet_unpack(con->packet);
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
