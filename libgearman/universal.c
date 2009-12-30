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
 * @ingroup state
 * @{
 */

gearman_universal_st *gearman_universal_create(gearman_universal_st *state, const gearman_options_t *options)
{
  assert(state);

  { // Set defaults on all options.
    state->options.allocated= false;
    state->options.dont_track_packets= false;
    state->options.non_blocking= false;
    state->options.stored_non_blocking= false;
  }

  if (options)
  {
    while (*options != GEARMAN_MAX)
    {
      /**
        @note Check for bad value, refactor gearman_add_options().
      */
      gearman_universal_add_options(state, *options);
      options++;
    }
  }

  state->verbose= 0;
  state->con_count= 0;
  state->packet_count= 0;
  state->pfds_size= 0;
  state->sending= 0;
  state->last_errno= 0;
  state->timeout= -1;
  state->con_list= NULL;
  state->packet_list= NULL;
  state->pfds= NULL;
  state->log_fn= NULL;
  state->log_context= NULL;
  state->event_watch_fn= NULL;
  state->event_watch_context= NULL;
  state->workload_malloc_fn= NULL;
  state->workload_malloc_context= NULL;
  state->workload_free_fn= NULL;
  state->workload_free_context= NULL;
  state->last_error[0]= 0;

  return state;
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

  /* Don't clone job or packet information, this is state information for
     old and active jobs/connections. */

  return destination;
}

void gearman_universal_free(gearman_universal_st *state)
{
  gearman_free_all_cons(state);
  gearman_free_all_packets(state);

  if (state->pfds != NULL)
    free(state->pfds);

  if (state->options.allocated)
  {
    assert(0);
    free(state);
  }
}

gearman_return_t gearman_universal_set_option(gearman_universal_st *state, gearman_options_t option, bool value)
{
  switch (option)
  {
  case GEARMAN_NON_BLOCKING:
    state->options.non_blocking= value;
    break;
  case GEARMAN_DONT_TRACK_PACKETS:
    state->options.dont_track_packets= value;
    break;
  case GEARMAN_MAX:
  default:
    return GEARMAN_INVALID_COMMAND;
  }

  return GEARMAN_SUCCESS;
}

int gearman_universal_timeout(gearman_universal_st *state)
{
  return state->timeout;
}

void gearman_universal_set_timeout(gearman_universal_st *state, int timeout)
{
  state->timeout= timeout;
}

void gearman_set_log_fn(gearman_universal_st *state, gearman_log_fn *function,
                        const void *context, gearman_verbose_t verbose)
{
  state->log_fn= function;
  state->log_context= context;
  state->verbose= verbose;
}

void gearman_set_event_watch_fn(gearman_universal_st *state,
                                gearman_event_watch_fn *function,
                                const void *context)
{
  state->event_watch_fn= function;
  state->event_watch_context= context;
}

void gearman_set_workload_malloc_fn(gearman_universal_st *state,
                                    gearman_malloc_fn *function,
                                    const void *context)
{
  state->workload_malloc_fn= function;
  state->workload_malloc_context= context;
}

void gearman_set_workload_free_fn(gearman_universal_st *state,
                                  gearman_free_fn *function,
                                  const void *context)
{
  state->workload_free_fn= function;
  state->workload_free_context= context;
}

void gearman_free_all_cons(gearman_universal_st *state)
{
  while (state->con_list != NULL)
    gearman_connection_free(state->con_list);
}

gearman_return_t gearman_flush_all(gearman_universal_st *state)
{
  gearman_connection_st *con;
  gearman_return_t ret;

  for (con= state->con_list; con != NULL; con= con->next)
  {
    if (con->events & POLLOUT)
      continue;

    ret= gearman_connection_flush(con);
    if (ret != GEARMAN_SUCCESS && ret != GEARMAN_IO_WAIT)
      return ret;
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_wait(gearman_universal_st *state)
{
  gearman_connection_st *con;
  struct pollfd *pfds;
  nfds_t x;
  int ret;
  gearman_return_t gret;

  if (state->pfds_size < state->con_count)
  {
    pfds= realloc(state->pfds, state->con_count * sizeof(struct pollfd));
    if (pfds == NULL)
    {
      gearman_universal_set_error(state, "gearman_wait", "realloc");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    state->pfds= pfds;
    state->pfds_size= state->con_count;
  }
  else
    pfds= state->pfds;

  x= 0;
  for (con= state->con_list; con != NULL; con= con->next)
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
    gearman_universal_set_error(state, "gearman_wait", "no active file descriptors");
    return GEARMAN_NO_ACTIVE_FDS;
  }

  while (1)
  {
    ret= poll(pfds, x, state->timeout);
    if (ret == -1)
    {
      if (errno == EINTR)
        continue;

      gearman_universal_set_error(state, "gearman_wait", "poll:%d", errno);
      state->last_errno= errno;
      return GEARMAN_ERRNO;
    }

    break;
  }

  if (ret == 0)
  {
    gearman_universal_set_error(state, "gearman_wait", "timeout reached");
    return GEARMAN_TIMEOUT;
  }

  x= 0;
  for (con= state->con_list; con != NULL; con= con->next)
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

gearman_connection_st *gearman_ready(gearman_universal_st *state)
{
  gearman_connection_st *con;

  /* We can't keep state between calls since connections may be removed during
     processing. If this list ever gets big, we may want something faster. */

  for (con= state->con_list; con != NULL; con= con->next)
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
  @note gearman_universal_push_blocking is only used for echo (and should be fixed
  when tricky flip/flop in IO is fixed).
*/
static inline void gearman_universal_push_blocking(gearman_universal_st *state)
{
  state->options.stored_non_blocking= state->options.non_blocking;
  state->options.non_blocking= false;
}

gearman_return_t gearman_echo(gearman_universal_st *state, const void *workload,
                              size_t workload_size)
{
  gearman_connection_st *con;
  gearman_packet_st packet;
  gearman_return_t ret;

  ret= gearman_packet_create_args(state, &packet, GEARMAN_MAGIC_REQUEST,
                                  GEARMAN_COMMAND_ECHO_REQ, &workload,
                                  &workload_size, 1);
  if (ret != GEARMAN_SUCCESS)
    return ret;

  gearman_universal_push_blocking(state);

  for (con= state->con_list; con != NULL; con= con->next)
  {
    ret= gearman_connection_send(con, &packet, true);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_packet_free(&packet);
      gearman_universal_pop_non_blocking(state);

      return ret;
    }

    (void)gearman_connection_recv(con, &(con->packet), &ret, true);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_packet_free(&packet);
      gearman_universal_pop_non_blocking(state);

      return ret;
    }

    if (con->packet.data_size != workload_size ||
        memcmp(workload, con->packet.data, workload_size))
    {
      gearman_packet_free(&(con->packet));
      gearman_packet_free(&packet);
      gearman_universal_pop_non_blocking(state);
      gearman_universal_set_error(state, "gearman_echo", "corruption during echo");

      return GEARMAN_ECHO_DATA_CORRUPTION;
    }

    gearman_packet_free(&(con->packet));
  }

  gearman_packet_free(&packet);
  gearman_universal_pop_non_blocking(state);

  return GEARMAN_SUCCESS;
}

void gearman_free_all_packets(gearman_universal_st *state)
{
  while (state->packet_list != NULL)
    gearman_packet_free(state->packet_list);
}

/*
 * Local Definitions
 */

void gearman_universal_set_error(gearman_universal_st *state, const char *function,
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

  if (state->log_fn == NULL)
  {
    if (size >= GEARMAN_MAX_ERROR_SIZE)
      size= GEARMAN_MAX_ERROR_SIZE - 1;

    memcpy(state->last_error, log_buffer, size + 1);
  }
  else
  {
    state->log_fn(log_buffer, GEARMAN_VERBOSE_FATAL,
                    (void *)state->log_context);
  }
}
