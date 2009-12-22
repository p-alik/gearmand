/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman State Definitions
 */

#include "common.h"

/**
 * @addtogroup gearman_state Static Gearman Declarations
 * @ingroup gearman
 * @{
 */

gearman_state_st *gearman_state_create(gearman_state_st *gearman)
{
  if (gearman == NULL)
  {
    gearman= malloc(sizeof(gearman_state_st));
    if (gearman == NULL)
      return NULL;

    gearman->options= GEARMAN_ALLOCATED;
  }
  else
    gearman->options= 0;

  gearman->verbose= 0;
  gearman->con_count= 0;
  gearman->packet_count= 0;
  gearman->pfds_size= 0;
  gearman->sending= 0;
  gearman->last_errno= 0;
  gearman->timeout= -1;
  gearman->con_list= NULL;
  gearman->packet_list= NULL;
  gearman->pfds= NULL;
  gearman->log_fn= NULL;
  gearman->log_context= NULL;
  gearman->event_watch_fn= NULL;
  gearman->event_watch_context= NULL;
  gearman->workload_malloc_fn= NULL;
  gearman->workload_malloc_context= NULL;
  gearman->workload_free_fn= NULL;
  gearman->workload_free_context= NULL;
  gearman->last_error[0]= 0;

  return gearman;
}

gearman_state_st *gearman_state_clone(gearman_state_st *gearman, const gearman_state_st *from)
{
  gearman_con_st *con;

  gearman= gearman_state_create(gearman);
  if (gearman == NULL)
    return NULL;

  gearman->options|= (from->options & (gearman_options_t)~GEARMAN_ALLOCATED);
  gearman->timeout= from->timeout;

  for (con= from->con_list; con != NULL; con= con->next)
  {
    if (gearman_clone_con(gearman, NULL, con) == NULL)
    {
      gearman_state_free(gearman);
      return NULL;
    }
  }

  /* Don't clone job or packet information, this is state information for
     old and active jobs/connections. */

  return gearman;
}

void gearman_state_free(gearman_state_st *gearman)
{
  gearman_free_all_cons(gearman);
  gearman_free_all_packets(gearman);

  if (gearman->pfds != NULL)
    free(gearman->pfds);

  if (gearman->options & GEARMAN_ALLOCATED)
    free(gearman);
}

gearman_options_t gearman_options(const gearman_state_st *gearman)
{
  return gearman->options;
}

void gearman_set_options(gearman_state_st *gearman, gearman_options_t options)
{
  gearman->options= options;
}

void gearman_add_options(gearman_state_st *gearman, gearman_options_t options)
{
  gearman->options|= options;
}

void gearman_remove_options(gearman_state_st *gearman, gearman_options_t options)
{
  gearman->options&= ~options;
}

int gearman_timeout(gearman_state_st *gearman)
{
  return gearman->timeout;
}

void gearman_set_timeout(gearman_state_st *gearman, int timeout)
{
  gearman->timeout= timeout;
}

void gearman_set_log_fn(gearman_state_st *gearman, gearman_log_fn *function,
                        const void *context, gearman_verbose_t verbose)
{
  gearman->log_fn= function;
  gearman->log_context= context;
  gearman->verbose= verbose;
}

void gearman_set_event_watch_fn(gearman_state_st *gearman,
                                gearman_event_watch_fn *function,
                                const void *context)
{
  gearman->event_watch_fn= function;
  gearman->event_watch_context= context;
}

void gearman_set_workload_malloc_fn(gearman_state_st *gearman,
                                    gearman_malloc_fn *function,
                                    const void *context)
{
  gearman->workload_malloc_fn= function;
  gearman->workload_malloc_context= context;
}

void gearman_set_workload_free_fn(gearman_state_st *gearman,
                                  gearman_free_fn *function,
                                  const void *context)
{
  gearman->workload_free_fn= function;
  gearman->workload_free_context= context;
}

/*
 * Connection related functions.
 */

gearman_con_st *gearman_add_con(gearman_state_st *gearman, gearman_con_st *con)
{
  if (con == NULL)
  {
    con= malloc(sizeof(gearman_con_st));
    if (con == NULL)
    {
      gearman_set_error(gearman, "gearman_add_con", "malloc");
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

gearman_con_st *gearman_add_con_args(gearman_state_st *gearman, gearman_con_st *con,
                                     const char *host, in_port_t port)
{
  con= gearman_add_con(gearman, con);
  if (con == NULL)
    return NULL;

  gearman_con_set_host(con, host);
  gearman_con_set_port(con, port);

  return con;
}

gearman_con_st *gearman_clone_con(gearman_state_st *gearman, gearman_con_st *con,
                                  const gearman_con_st *from)
{
  con= gearman_add_con(gearman, con);
  if (con == NULL)
    return NULL;

  con->options|= (from->options &
                  (gearman_con_options_t)~GEARMAN_CON_ALLOCATED);
  strcpy(con->host, from->host);
  con->port= from->port;

  return con;
}

void gearman_con_free(gearman_con_st *con)
{
  if (con->fd != -1)
    gearman_con_close(con);

  gearman_con_reset_addrinfo(con);

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

void gearman_free_all_cons(gearman_state_st *gearman)
{
  while (gearman->con_list != NULL)
    gearman_con_free(gearman->con_list);
}

gearman_return_t gearman_flush_all(gearman_state_st *gearman)
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

gearman_return_t gearman_send_all(gearman_state_st *gearman,
                                  const gearman_packet_st *packet)
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
    while ((con= gearman_ready(gearman)) != NULL)
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

    ret= gearman_wait(gearman);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman->options= options;
      return ret;
    }
  }

  gearman->options= options;
  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_wait(gearman_state_st *gearman)
{
  gearman_con_st *con;
  struct pollfd *pfds;
  nfds_t x;
  int ret;
  gearman_return_t gret;

  if (gearman->pfds_size < gearman->con_count)
  {
    pfds= realloc(gearman->pfds, gearman->con_count * sizeof(struct pollfd));
    if (pfds == NULL)
    {
      gearman_set_error(gearman, "gearman_wait", "realloc");
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
    gearman_set_error(gearman, "gearman_wait", "no active file descriptors");
    return GEARMAN_NO_ACTIVE_FDS;
  }

  while (1)
  {
    ret= poll(pfds, x, gearman->timeout);
    if (ret == -1)
    {
      if (errno == EINTR)
        continue;

      gearman_set_error(gearman, "gearman_wait", "poll:%d", errno);
      gearman->last_errno= errno;
      return GEARMAN_ERRNO;
    }

    break;
  }

  if (ret == 0)
  {
    gearman_set_error(gearman, "gearman_wait", "timeout reached");
    return GEARMAN_TIMEOUT;
  }

  x= 0;
  for (con= gearman->con_list; con != NULL; con= con->next)
  {
    if (con->events == 0)
      continue;

    gret= gearman_con_set_revents(con, pfds[x].revents);
    if (gret != GEARMAN_SUCCESS)
      return gret;

    x++;
  }

  return GEARMAN_SUCCESS;
}

gearman_con_st *gearman_ready(gearman_state_st *gearman)
{
  gearman_con_st *con;

  /* We can't keep state between calls since connections may be removed during
     processing. If this list ever gets big, we may want something faster. */

  for (con= gearman->con_list; con != NULL; con= con->next)
  {
    if (con->options & GEARMAN_CON_READY)
    {
      con->options&= (gearman_con_options_t)~GEARMAN_CON_READY;
      return con;
    }
  }

  return NULL;
}

gearman_return_t gearman_echo(gearman_state_st *gearman, const void *workload,
                              size_t workload_size)
{
  gearman_con_st *con;
  gearman_options_t options= gearman->options;
  gearman_packet_st packet;
  gearman_return_t ret;

  ret= gearman_add_packet_args(gearman, &packet, GEARMAN_MAGIC_REQUEST,
                               GEARMAN_COMMAND_ECHO_REQ, &workload,
                               &workload_size, 1);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  gearman->options&= (gearman_con_options_t)~GEARMAN_NON_BLOCKING;

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
      gearman_set_error(gearman, "gearman_echo", "corruption during echo");
      return GEARMAN_ECHO_DATA_CORRUPTION;
    }

    gearman_packet_free(&(con->packet));
  }

  gearman_packet_free(&packet);
  gearman->options= options;
  return GEARMAN_SUCCESS;
}

/*
 * Packet related functions.
 */

gearman_packet_st *gearman_add_packet(gearman_state_st *gearman,
                                      gearman_packet_st *packet)
{
  if (packet == NULL)
  {
    packet= malloc(sizeof(gearman_packet_st));
    if (packet == NULL)
    {
      gearman_set_error(gearman, "gearman_add_packet", "malloc");
      return NULL;
    }

    packet->options= GEARMAN_PACKET_ALLOCATED;
  }
  else
    packet->options= 0;

  packet->magic= 0;
  packet->command= 0;
  packet->argc= 0;
  packet->args_size= 0;
  packet->data_size= 0;
  packet->gearman= gearman;

  if (!(gearman->options & GEARMAN_DONT_TRACK_PACKETS))
  {
    if (gearman->packet_list != NULL)
      gearman->packet_list->prev= packet;
    packet->next= gearman->packet_list;
    packet->prev= NULL;
    gearman->packet_list= packet;
    gearman->packet_count++;
  }

  packet->args= NULL;
  packet->data= NULL;

  return packet;
}

gearman_return_t gearman_add_packet_args(gearman_state_st *gearman,
                                         gearman_packet_st *packet,
                                         gearman_magic_t magic,
                                         gearman_command_t command,
                                         const void *args[],
                                         const size_t args_size[],
                                         size_t args_count)
{
  gearman_return_t ret;
  size_t x;

  packet= gearman_add_packet(gearman, packet);
  if (packet == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  packet->magic= magic;
  packet->command= command;

  for (x= 0; x < args_count; x++)
  {
    ret= gearman_packet_add_arg(packet, args[x], args_size[x]);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_packet_free(packet);
      return ret;
    }
  }

  return gearman_packet_pack_header(packet);
}

void gearman_packet_free(gearman_packet_st *packet)
{
  if (packet->args != packet->args_buffer && packet->args != NULL)
    free(packet->args);

  if (packet->options & GEARMAN_PACKET_FREE_DATA && packet->data != NULL)
  {
    if (packet->gearman->workload_free_fn == NULL)
      free((void *)packet->data);
    else
    {
      packet->gearman->workload_free_fn((void *)(packet->data),
                                (void *)packet->gearman->workload_free_context);
    }
  }

  if (!(packet->gearman->options & GEARMAN_DONT_TRACK_PACKETS))
  {
    if (packet->gearman->packet_list == packet)
      packet->gearman->packet_list= packet->next;
    if (packet->prev != NULL)
      packet->prev->next= packet->next;
    if (packet->next != NULL)
      packet->next->prev= packet->prev;
    packet->gearman->packet_count--;
  }

  if (packet->options & GEARMAN_PACKET_ALLOCATED)
    free(packet);
}

void gearman_free_all_packets(gearman_state_st *gearman)
{
  while (gearman->packet_list != NULL)
    gearman_packet_free(gearman->packet_list);
}

/*
 * Local Definitions
 */

void gearman_set_error(gearman_state_st *gearman, const char *function,
                       const char *format, ...)
{
  size_t size;
  char *ptr;
  char log_buffer[GEARMAN_MAX_ERROR_SIZE];
  va_list args;

  size= strlen(function);
  ptr= memcpy(log_buffer, function, size);
  ptr+= size;
  ptr[0]= ':';
  size++;
  ptr++;

  va_start(args, format);
  size+= (size_t)vsnprintf(ptr, GEARMAN_MAX_ERROR_SIZE - size, format, args);
  va_end(args);

  if (gearman->log_fn == NULL)
  {
    if (size >= GEARMAN_MAX_ERROR_SIZE)
      size= GEARMAN_MAX_ERROR_SIZE - 1;

    memcpy(gearman->last_error, log_buffer, size + 1);
  }
  else
  {
    gearman->log_fn(log_buffer, GEARMAN_VERBOSE_FATAL,
                    (void *)gearman->log_context);
  }
}

void gearman_log(gearman_state_st *gearman, gearman_verbose_t verbose,
                 const char *format, va_list args)
{
  char log_buffer[GEARMAN_MAX_ERROR_SIZE];

  if (gearman->log_fn == NULL)
  {
    printf("%5s: ", gearman_verbose_name(verbose));
    vprintf(format, args);
    printf("\n");
  }
  else
  {
    vsnprintf(log_buffer, GEARMAN_MAX_ERROR_SIZE, format, args);
    gearman->log_fn(log_buffer, verbose, (void *)gearman->log_context);
  }
}

gearman_return_t gearman_parse_servers(const char *servers,
                                       gearman_parse_server_fn *function,
                                       const void *context)
{
  const char *ptr= servers;
  size_t x;
  char host[NI_MAXHOST];
  char port[NI_MAXSERV];
  gearman_return_t ret;

  if (ptr == NULL)
    return (*function)(NULL, 0, (void *)context);

  while (1)
  {
    x= 0;

    while (*ptr != 0 && *ptr != ',' && *ptr != ':')
    {
      if (x < (NI_MAXHOST - 1))
        host[x++]= *ptr;

      ptr++;
    }

    host[x]= 0;

    if (*ptr == ':')
    {
      ptr++;
      x= 0;

      while (*ptr != 0 && *ptr != ',')
      {
        if (x < (NI_MAXSERV - 1))
          port[x++]= *ptr;

        ptr++;
      }

      port[x]= 0;
    }
    else
      port[0]= 0;

    ret= (*function)(host, (in_port_t)atoi(port), (void *)context);
    if (ret != GEARMAN_SUCCESS)
      return ret;

    if (*ptr == 0)
      break;

    ptr++;
  }

  return GEARMAN_SUCCESS;
}
