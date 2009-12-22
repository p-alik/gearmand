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

gearman_state_st *gearman_state_create(gearman_state_st *gearman, gearman_options_t *options)
{
  if (gearman == NULL)
  {
    gearman= malloc(sizeof(gearman_state_st));
    if (gearman == NULL)
      return NULL;

    gearman->options.allocated= true;
  }
  else
  {
    gearman->options.allocated= false;
  }

  { // Set defaults on all options.
    gearman->options.dont_track_packets= false;
    gearman->options.non_blocking= false;
    gearman->options.stored_non_blocking= false;
  }

  if (options)
  {
    while (*options != GEARMAN_MAX)
    {
      /**
        @note Check for bad value, refactor gearman_add_options().
      */
      gearman_add_options(gearman, *options);
      options++;
    }
  }

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

gearman_state_st *gearman_state_clone(gearman_state_st *destination, const gearman_state_st *source)
{
  gearman_connection_st *con;

  destination= gearman_state_create(destination, NULL);

  if (! source || ! destination)
  {
    return destination;
  }

  (void)gearman_set_option(destination, GEARMAN_NON_BLOCKING, source->options.non_blocking);
  (void)gearman_set_option(destination, GEARMAN_DONT_TRACK_PACKETS, source->options.dont_track_packets);

  destination->timeout= source->timeout;

  for (con= source->con_list; con != NULL; con= con->next)
  {
    if (gearman_connection_clone(destination, NULL, con) == NULL)
    {
      gearman_state_free(destination);
      return NULL;
    }
  }

  /* Don't clone job or packet information, this is state information for
     old and active jobs/connections. */

  return destination;
}

void gearman_state_free(gearman_state_st *gearman)
{
  gearman_free_all_cons(gearman);
  gearman_free_all_packets(gearman);

  if (gearman->pfds != NULL)
    free(gearman->pfds);

  if (gearman->options.allocated)
    free(gearman);
}

gearman_return_t gearman_set_option(gearman_state_st *gearman, gearman_options_t option, bool value)
{
  switch (option)
  {
  case GEARMAN_NON_BLOCKING:
    gearman->options.non_blocking= value;
    break;
  case GEARMAN_DONT_TRACK_PACKETS:
    gearman->options.dont_track_packets= value;
    break;
  case GEARMAN_MAX:
  default:
    return GEARMAN_INVALID_COMMAND;
  }

  return GEARMAN_SUCCESS;
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

void gearman_free_all_cons(gearman_state_st *gearman)
{
  while (gearman->con_list != NULL)
    gearman_connection_free(gearman->con_list);
}

gearman_return_t gearman_flush_all(gearman_state_st *gearman)
{
  gearman_connection_st *con;
  gearman_return_t ret;

  for (con= gearman->con_list; con != NULL; con= con->next)
  {
    if (con->events & POLLOUT)
      continue;

    ret= gearman_connection_flush(con);
    if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
      return ret;
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_wait(gearman_state_st *gearman)
{
  gearman_connection_st *con;
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

    gret= gearman_connection_set_revents(con, pfds[x].revents);
    if (gret != GEARMAN_SUCCESS)
      return gret;

    x++;
  }

  return GEARMAN_SUCCESS;
}

gearman_connection_st *gearman_ready(gearman_state_st *gearman)
{
  gearman_connection_st *con;

  /* We can't keep state between calls since connections may be removed during
     processing. If this list ever gets big, we may want something faster. */

  for (con= gearman->con_list; con != NULL; con= con->next)
  {
    if (con->options.ready)
    {
      con->options.ready= false;
      return con;
    }
  }

  return NULL;
}

/**
  @note gearman_state_push_blocking is only used for echo (and should be fixed
  when tricky flip/flop in IO is fixed).
*/
static inline void gearman_state_push_blocking(gearman_state_st *gearman)
{
  gearman->options.stored_non_blocking= gearman->options.non_blocking;
  gearman->options.non_blocking= false;
}

gearman_return_t gearman_echo(gearman_state_st *gearman, const void *workload,
                              size_t workload_size)
{
  gearman_connection_st *con;
  gearman_packet_st packet;
  gearman_return_t ret;

  ret= gearman_packet_create_args(gearman, &packet, GEARMAN_MAGIC_REQUEST,
                                  GEARMAN_COMMAND_ECHO_REQ, &workload,
                                  &workload_size, 1);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  gearman_state_push_blocking(gearman);

  for (con= gearman->con_list; con != NULL; con= con->next)
  {
    ret= gearman_connection_send(con, &packet, true);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_packet_free(&packet);
      gearman_state_pop_non_blocking(gearman);

      return ret;
    }

    (void)gearman_connection_recv(con, &(con->packet), &ret, true);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_packet_free(&packet);
      gearman_state_pop_non_blocking(gearman);

      return ret;
    }

    if (con->packet.data_size != workload_size ||
        memcmp(workload, con->packet.data, workload_size))
    {
      gearman_packet_free(&(con->packet));
      gearman_packet_free(&packet);
      gearman_state_pop_non_blocking(gearman);
      gearman_set_error(gearman, "gearman_echo", "corruption during echo");

      return GEARMAN_ECHO_DATA_CORRUPTION;
    }

    gearman_packet_free(&(con->packet));
  }

  gearman_packet_free(&packet);
  gearman_state_pop_non_blocking(gearman);

  return GEARMAN_SUCCESS;
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
