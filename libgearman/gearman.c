/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Gearman core definitions
 */

#include "common.h"

/*
 * Private declarations
 */

/**
 * @addtogroup gearman_private Private Functions
 * @ingroup gearman
 * @{
 */

/**
 * Names of the verbose levels provided.
 */
static const char *_verbose_name[GEARMAN_VERBOSE_MAX]=
{
  "FATAL",
  "ERROR",
  "INFO",
  "DEBUG",
  "CRAZY"
};

/** @} */

/*
 * Public definitions
 */

const char *gearman_version(void)
{
    return PACKAGE_VERSION;
}

const char *gearman_bugreport(void)
{
    return PACKAGE_BUGREPORT;
}

const char *gearman_verbose_name(gearman_verbose_t verbose)
{
  if (verbose >= GEARMAN_VERBOSE_MAX)
    return "UNKNOWN";

  return _verbose_name[verbose];
}

gearman_st *gearman_create(gearman_st *gearman)
{
  if (gearman == NULL)
  {
    gearman= malloc(sizeof(gearman_st));
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

gearman_st *gearman_clone(gearman_st *gearman, const gearman_st *from)
{
  gearman_con_st *con;

  gearman= gearman_create(gearman);
  if (gearman == NULL)
    return NULL;

  gearman->options|= (from->options & (gearman_options_t)~GEARMAN_ALLOCATED);

  for (con= from->con_list; con != NULL; con= con->next)
  {
    if (gearman_con_clone(gearman, NULL, con) == NULL)
    {
      gearman_free(gearman);
      return NULL;
    }
  }

  /* Don't clone job or packet information, this is state information for
     old and active jobs/connections. */

  return gearman;
}

void gearman_free(gearman_st *gearman)
{
  gearman_con_free_all(gearman);
  gearman_packet_free_all(gearman);

  if (gearman->pfds != NULL)
    free(gearman->pfds);

  if (gearman->options & GEARMAN_ALLOCATED)
    free(gearman);
}

const char *gearman_error(const gearman_st *gearman)
{
  return (const char *)(gearman->last_error);
}

int gearman_errno(const gearman_st *gearman)
{
  return gearman->last_errno;
}

gearman_options_t gearman_options(const gearman_st *gearman)
{
  return gearman->options;
}

void gearman_set_options(gearman_st *gearman, gearman_options_t options)
{
  gearman->options= options;
}

void gearman_add_options(gearman_st *gearman, gearman_options_t options)
{
  gearman->options|= options;
}

void gearman_remove_options(gearman_st *gearman, gearman_options_t options)
{
  gearman->options&= ~options;
}

void gearman_set_log_fn(gearman_st *gearman, gearman_log_fn *function,
                        const void *context, gearman_verbose_t verbose)
{
  gearman->log_fn= function;
  gearman->log_context= context;
  gearman->verbose= verbose;
}

void gearman_set_event_watch_fn(gearman_st *gearman,
                                gearman_event_watch_fn *function,
                                const void *context)
{
  gearman->event_watch_fn= function;
  gearman->event_watch_context= context;
}

void gearman_set_workload_malloc_fn(gearman_st *gearman,
                                    gearman_malloc_fn *function,
                                    const void *context)
{
  gearman->workload_malloc_fn= function;
  gearman->workload_malloc_context= context;
}

void gearman_set_workload_free_fn(gearman_st *gearman,
                                  gearman_free_fn *function,
                                  const void *context)
{
  gearman->workload_free_fn= function;
  gearman->workload_free_context= context;
}

/*
 * Connection related functions.
 */

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
  GEARMAN_LIST_ADD(gearman->con, con,)
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

gearman_con_st *gearman_con_clone(gearman_st *gearman, gearman_con_st *con,
                                  const gearman_con_st *from)
{
  con= gearman_con_create(gearman, con);
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
    (*con->protocol_context_free_fn)(con, (void *)con->protocol_context);

  GEARMAN_LIST_DEL(con->gearman->con, con,)

  if (con->options & GEARMAN_CON_PACKET_IN_USE)
    gearman_packet_free(&(con->packet));

  if (con->options & GEARMAN_CON_ALLOCATED)
    free(con);
}

void gearman_con_free_all(gearman_st *gearman)
{
  while (gearman->con_list != NULL)
    gearman_con_free(gearman->con_list);
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

    ret= gearman_con_wait(gearman, -1);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman->options= options;
      return ret;
    }
  }

  gearman->options= options;
  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_con_wait(gearman_st *gearman, int timeout)
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

  while (1)
  {
    ret= poll(pfds, x, timeout);
    if (ret == -1)
    {
      if (errno == EINTR)
        continue;

      GEARMAN_ERROR_SET(gearman, "gearman_con_wait", "poll:%d", errno)
      gearman->last_errno= errno;
      return GEARMAN_ERRNO;
    }

    break;
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

gearman_con_st *gearman_con_ready(gearman_st *gearman)
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
 * Packet related functions.
 */

gearman_packet_st *gearman_packet_create(gearman_st *gearman,
                                         gearman_packet_st *packet)
{
  if (packet == NULL)
  {
    packet= malloc(sizeof(gearman_packet_st));
    if (packet == NULL)
    {
      GEARMAN_ERROR_SET(gearman, "gearman_packet_create", "malloc")
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
    GEARMAN_LIST_ADD(gearman->packet, packet,)

  packet->args= NULL;
  packet->data= NULL;

  return packet;
}

gearman_return_t gearman_packet_add(gearman_st *gearman,
                                    gearman_packet_st *packet,
                                    gearman_magic_t magic,
                                    gearman_command_t command,
                                    const void *arg, ...)
{
  va_list ap;
  size_t arg_size;
  gearman_return_t ret;

  packet= gearman_packet_create(gearman, packet);
  if (packet == NULL)
    return GEARMAN_MEMORY_ALLOCATION_FAILURE;

  packet->magic= magic;
  packet->command= command;

  va_start(ap, arg);

  while (arg != NULL)
  {
    arg_size = va_arg(ap, size_t);

    ret= gearman_packet_add_arg(packet, arg, arg_size);
    if (ret != GEARMAN_SUCCESS)
    {
      va_end(ap);
      gearman_packet_free(packet);
      return ret;
    }

    arg = va_arg(ap, void *);
  }

  va_end(ap);

  return gearman_packet_pack_header(packet);
}

void gearman_packet_free(gearman_packet_st *packet)
{
  if (packet->args != packet->args_buffer && packet->args != NULL)
    free(packet->args);

  if (packet->options & GEARMAN_PACKET_FREE_DATA && packet->data != NULL)
  {
    if (packet->gearman->workload_free_fn == NULL)
      free((void *)(packet->data));
    else
    {
      packet->gearman->workload_free_fn((void *)(packet->data),
                              (void *)(packet->gearman->workload_free_context));
    }
  }

  if (!(packet->gearman->options & GEARMAN_DONT_TRACK_PACKETS))
    GEARMAN_LIST_DEL(packet->gearman->packet, packet,)

  if (packet->options & GEARMAN_PACKET_ALLOCATED)
    free(packet);
}

void gearman_packet_free_all(gearman_st *gearman)
{
  while (gearman->packet_list != NULL)
    gearman_packet_free(gearman->packet_list);
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
