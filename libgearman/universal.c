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
 * @addtogroup gearman_universal Static Gearman Declarations
 * @ingroup universal
 * @{
 */

gearman_universal_st *gearman_universal_create(gearman_universal_st *universal, const gearman_universal_options_t *options)
{
  assert(universal);

  { // Set defaults on all options.
    universal->options.allocated= false;
    universal->options.dont_track_packets= false;
    universal->options.non_blocking= false;
    universal->options.stored_non_blocking= false;
  }

  if (options)
  {
    while (*options != GEARMAN_MAX)
    {
      /**
        @note Check for bad value, refactor gearman_add_options().
      */
      gearman_universal_add_options(universal, *options);
      options++;
    }
  }

  universal->verbose= 0;
  universal->con_count= 0;
  universal->packet_count= 0;
  universal->pfds_size= 0;
  universal->sending= 0;
  universal->last_errno= 0;
  universal->timeout= -1;
  universal->con_list= NULL;
  universal->packet_list= NULL;
  universal->pfds= NULL;
  universal->log_fn= NULL;
  universal->log_context= NULL;
  universal->event_watch_fn= NULL;
  universal->event_watch_context= NULL;
  universal->workload_malloc_fn= NULL;
  universal->workload_malloc_context= NULL;
  universal->workload_free_fn= NULL;
  universal->workload_free_context= NULL;
  universal->last_error[0]= 0;

  return universal;
}

gearman_universal_st *gearman_universal_clone(gearman_universal_st *destination, const gearman_universal_st *source)
{
  gearman_universal_st *check;
  gearman_connection_st *con;

  assert(destination);
  assert(source);

  if (! destination || ! source)
    return NULL;

  check= gearman_universal_create(destination, NULL);

  if (! check)
  {
    return destination;
  }

  (void)gearman_universal_set_option(destination, GEARMAN_NON_BLOCKING, source->options.non_blocking);
  (void)gearman_universal_set_option(destination, GEARMAN_DONT_TRACK_PACKETS, source->options.dont_track_packets);

  destination->timeout= source->timeout;

  for (con= source->con_list; con != NULL; con= con->next)
  {
    if (gearman_connection_clone(destination, NULL, con) == NULL)
    {
      gearman_universal_free(destination);
      return NULL;
    }
  }

  /* Don't clone job or packet information, this is universal information for
    old and active jobs/connections. */

  return destination;
}

void gearman_universal_free(gearman_universal_st *universal)
{
  gearman_free_all_cons(universal);
  gearman_free_all_packets(universal);

  if (universal->pfds != NULL)
    free(universal->pfds);

  if (universal->options.allocated)
  {
    assert(0);
    free(universal);
  }
}

gearman_return_t gearman_universal_set_option(gearman_universal_st *universal, gearman_universal_options_t option, bool value)
{
  switch (option)
  {
  case GEARMAN_NON_BLOCKING:
    universal->options.non_blocking= value;
    break;
  case GEARMAN_DONT_TRACK_PACKETS:
    universal->options.dont_track_packets= value;
    break;
  case GEARMAN_MAX:
  default:
    return GEARMAN_INVALID_COMMAND;
  }

  return GEARMAN_SUCCESS;
}

int gearman_universal_timeout(gearman_universal_st *universal)
{
  return universal->timeout;
}

void gearman_universal_set_timeout(gearman_universal_st *universal, int timeout)
{
  universal->timeout= timeout;
}

void gearman_set_log_fn(gearman_universal_st *universal, gearman_log_fn *function,
                        void *context, gearman_verbose_t verbose)
{
  universal->log_fn= function;
  universal->log_context= context;
  universal->verbose= verbose;
}

void gearman_set_event_watch_fn(gearman_universal_st *universal,
                                gearman_event_watch_fn *function,
                                void *context)
{
  universal->event_watch_fn= function;
  universal->event_watch_context= context;
}

void gearman_set_workload_malloc_fn(gearman_universal_st *universal,
                                    gearman_malloc_fn *function,
                                    void *context)
{
  universal->workload_malloc_fn= function;
  universal->workload_malloc_context= context;
}

void gearman_set_workload_free_fn(gearman_universal_st *universal,
                                  gearman_free_fn *function,
                                  void *context)
{
  universal->workload_free_fn= function;
  universal->workload_free_context= context;
}

void gearman_free_all_cons(gearman_universal_st *universal)
{
  while (universal->con_list != NULL)
    gearman_connection_free(universal->con_list);
}

gearman_return_t gearman_flush_all(gearman_universal_st *universal)
{
  gearman_connection_st *con;
  gearman_return_t ret;

  for (con= universal->con_list; con != NULL; con= con->next)
  {
    if (con->events & POLLOUT)
      continue;

    ret= gearman_connection_flush(con);
    if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
      return ret;
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_wait(gearman_universal_st *universal)
{
  gearman_connection_st *con;
  struct pollfd *pfds;
  nfds_t x;
  int ret;
  gearman_return_t gret;

  if (universal->pfds_size < universal->con_count)
  {
    pfds= realloc(universal->pfds, universal->con_count * sizeof(struct pollfd));
    if (pfds == NULL)
    {
      gearman_universal_set_error(universal, "gearman_wait", "realloc");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    universal->pfds= pfds;
    universal->pfds_size= universal->con_count;
  }
  else
    pfds= universal->pfds;

  x= 0;
  for (con= universal->con_list; con != NULL; con= con->next)
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
    gearman_universal_set_error(universal, "gearman_wait", "no active file descriptors");
    return GEARMAN_NO_ACTIVE_FDS;
  }

  while (1)
  {
    ret= poll(pfds, x, universal->timeout);
    if (ret == -1)
    {
      if (errno == EINTR)
        continue;

      gearman_universal_set_error(universal, "gearman_wait", "poll:%d", errno);
      universal->last_errno= errno;
      return GEARMAN_ERRNO;
    }

    break;
  }

  if (ret == 0)
  {
    gearman_universal_set_error(universal, "gearman_wait", "timeout reached");
    return GEARMAN_TIMEOUT;
  }

  x= 0;
  for (con= universal->con_list; con != NULL; con= con->next)
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

gearman_connection_st *gearman_ready(gearman_universal_st *universal)
{
  gearman_connection_st *con;

  /* We can't keep universal between calls since connections may be removed during
    processing. If this list ever gets big, we may want something faster. */

  for (con= universal->con_list; con != NULL; con= con->next)
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
  @note _push_blocking is only used for echo (and should be fixed
  when tricky flip/flop in IO is fixed).
*/
static inline void _push_blocking(gearman_universal_st *universal, bool *orig_block_universal)
{
  *orig_block_universal= universal->options.non_blocking;
  universal->options.non_blocking= false;
}

static inline void _pop_non_blocking(gearman_universal_st *universal, bool orig_block_universal)
{
  universal->options.non_blocking= orig_block_universal;
}

gearman_return_t gearman_echo(gearman_universal_st *universal,
                              const void *workload,
                              size_t workload_size)
{
  gearman_connection_st *con;
  gearman_packet_st packet;
  gearman_return_t ret;
  bool orig_block_universal;

  ret= gearman_packet_create_args(universal, &packet, GEARMAN_MAGIC_REQUEST,
                                  GEARMAN_COMMAND_ECHO_REQ,
                                  &workload, &workload_size, 1);
  if (ret != GEARMAN_SUCCESS)
  {
    return ret;
  }

  _push_blocking(universal, &orig_block_universal);

  for (con= universal->con_list; con != NULL; con= con->next)
  {
    gearman_packet_st *packet_ptr;

    ret= gearman_connection_send(con, &packet, true);
    if (ret != GEARMAN_SUCCESS)
    {
      goto exit;
    }

    packet_ptr= gearman_connection_recv(con, &(con->packet), &ret, true);
    if (ret != GEARMAN_SUCCESS)
    {
      goto exit;
    }

    assert(packet_ptr);

    if (con->packet.data_size != workload_size ||
        memcmp(workload, con->packet.data, workload_size))
    {
      gearman_packet_free(&(con->packet));
      gearman_universal_set_error(universal, "gearman_echo", "corruption during echo");

      ret= GEARMAN_ECHO_DATA_CORRUPTION;
      goto exit;
    }

    gearman_packet_free(&(con->packet));
  }

  ret= GEARMAN_SUCCESS;

exit:
  gearman_packet_free(&packet);
  _pop_non_blocking(universal, orig_block_universal);

  return ret;
}

void gearman_free_all_packets(gearman_universal_st *universal)
{
  while (universal->packet_list != NULL)
    gearman_packet_free(universal->packet_list);
}

/*
 * Local Definitions
 */

void gearman_universal_set_error(gearman_universal_st *universal, const char *function,
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

  if (universal->log_fn == NULL)
  {
    if (size >= GEARMAN_MAX_ERROR_SIZE)
      size= GEARMAN_MAX_ERROR_SIZE - 1;

    memcpy(universal->last_error, log_buffer, size + 1);
  }
  else
  {
    universal->log_fn(log_buffer, GEARMAN_VERBOSE_FATAL,
                      (void *)universal->log_context);
  }
}
