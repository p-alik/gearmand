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

#include <libgearman/common.h>
#include <libgearman/connection.h>
#include <libgearman/packet.hpp>
#include <libgearman/universal.hpp>

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void gearman_universal_initialize(gearman_universal_st &self, const gearman_universal_options_t *options)
{
  { // Set defaults on all options.
    self.options.dont_track_packets= false;
    self.options.non_blocking= false;
    self.options.stored_non_blocking= false;
  }

  if (options)
  {
    while (*options != GEARMAN_MAX)
    {
      /**
        @note Check for bad value, refactor gearman_add_options().
      */
      gearman_universal_add_options(self, *options);
      options++;
    }
  }

  self.verbose= GEARMAN_VERBOSE_NEVER;
  self.con_count= 0;
  self.packet_count= 0;
  self.pfds_size= 0;
  self.sending= 0;
  self.timeout= -1;
  self.con_list= NULL;
  self.packet_list= NULL;
  self.pfds= NULL;
  self.log_fn= NULL;
  self.log_context= NULL;
  self.workload_malloc_fn= NULL;
  self.workload_malloc_context= NULL;
  self.workload_free_fn= NULL;
  self.workload_free_context= NULL;
  self.error.rc= GEARMAN_SUCCESS;
  self.error.last_errno= 0;
  self.error.last_error[0]= 0;
}

void gearman_universal_clone(gearman_universal_st &destination, const gearman_universal_st &source)
{
  gearman_universal_initialize(destination);

  (void)gearman_universal_set_option(destination, GEARMAN_NON_BLOCKING, source.options.non_blocking);
  (void)gearman_universal_set_option(destination, GEARMAN_DONT_TRACK_PACKETS, source.options.dont_track_packets);

  destination.timeout= source.timeout;

  for (gearman_connection_st *con= source.con_list; con; con= con->next)
  {
    if (not gearman_connection_copy(destination, *con))
    {
      gearman_universal_free(destination);
      return;
    }
  }
}

void gearman_universal_free(gearman_universal_st &universal)
{
  gearman_free_all_cons(&universal);
  gearman_free_all_packets(universal);

  if (universal.pfds)
  {
    // created realloc()
    free(universal.pfds);
  }
}

gearman_return_t gearman_universal_set_option(gearman_universal_st &self, gearman_universal_options_t option, bool value)
{
  switch (option)
  {
  case GEARMAN_NON_BLOCKING:
    self.options.non_blocking= value;
    break;
  case GEARMAN_DONT_TRACK_PACKETS:
    self.options.dont_track_packets= value;
    break;
  case GEARMAN_MAX:
  default:
    return GEARMAN_INVALID_COMMAND;
  }

  return GEARMAN_SUCCESS;
}

int gearman_universal_timeout(gearman_universal_st &self)
{
  return self.timeout;
}

void gearman_universal_set_timeout(gearman_universal_st &self, int timeout)
{
  self.timeout= timeout;
}

void gearman_set_log_fn(gearman_universal_st &self, gearman_log_fn *function,
                        void *context, gearman_verbose_t verbose)
{
  self.log_fn= function;
  self.log_context= context;
  self.verbose= verbose;
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
  while (universal->con_list)
    gearman_connection_free(universal->con_list);
}

gearman_return_t gearman_flush_all(gearman_universal_st *universal)
{
  for (gearman_connection_st *con= universal->con_list; con != NULL; con= con->next)
  {
    if (con->events & POLLOUT)
      continue;

    gearman_return_t ret= con->flush();
    if (gearman_failed(ret) and ret != GEARMAN_IO_WAIT)
      return ret;
  }

  return GEARMAN_SUCCESS;
}

gearman_return_t gearman_wait(gearman_universal_st& universal)
{
  struct pollfd *pfds;

  if (universal.pfds_size < universal.con_count)
  {
    pfds= static_cast<pollfd*>(realloc(universal.pfds, universal.con_count * sizeof(struct pollfd)));
    if (pfds == NULL)
    {
      gearman_perror(universal, "pollfd realloc");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    universal.pfds= pfds;
    universal.pfds_size= universal.con_count;
  }
  else
  {
    pfds= universal.pfds;
  }

  nfds_t x= 0;
  for (gearman_connection_st *con= universal.con_list; con != NULL; con= con->next)
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
    gearman_error(&universal, GEARMAN_NO_ACTIVE_FDS, "no active file descriptors");
    return GEARMAN_NO_ACTIVE_FDS;
  }

  int ret;
  while (1)
  {
    ret= poll(pfds, x, universal.timeout);
    if (ret == -1)
    {
      if (errno == EINTR)
        continue;

      gearman_perror(universal, "poll");
      return GEARMAN_ERRNO;
    }

    break;
  }

  if (ret == 0)
  {
    gearman_error(&universal, GEARMAN_TIMEOUT, "timeout reached");
    return GEARMAN_TIMEOUT;
  }

  x= 0;
  for (gearman_connection_st *con= universal.con_list; con != NULL; con= con->next)
  {
    if (con->events == 0)
      continue;

    gearman_return_t gret= gearman_connection_set_revents(con, pfds[x].revents);
    if (gearman_failed(gret))
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

  ret= gearman_packet_create_args(*universal, &packet, GEARMAN_MAGIC_REQUEST,
                                  GEARMAN_COMMAND_ECHO_REQ,
                                  &workload, &workload_size, 1);
  if (gearman_failed(ret))
  {
    return ret;
  }

  _push_blocking(universal, &orig_block_universal);

  for (con= universal->con_list; con != NULL; con= con->next)
  {
    gearman_packet_st *packet_ptr;

    ret= con->send(&packet, true);
    if (gearman_failed(ret))
    {
      goto exit;
    }

    packet_ptr= con->recv(&(con->packet), &ret, true);
    if (gearman_failed(ret))
    {
      goto exit;
    }

    assert(packet_ptr);

    if (con->packet.data_size != workload_size ||
        memcmp(workload, con->packet.data, workload_size))
    {
      gearman_packet_free(&(con->packet));
      gearman_error(universal, GEARMAN_ECHO_DATA_CORRUPTION, "corruption during echo");

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

bool gearman_request_option(gearman_universal_st &universal,
                            gearman_string_t &option)
{
  gearman_connection_st *con;
  gearman_packet_st packet;
  gearman_return_t ret;
  bool orig_block_universal;

  const void *args[]= { gearman_c_str(option) };
  size_t args_size[]= { gearman_size(option) };

  ret= gearman_packet_create_args(universal, &packet, GEARMAN_MAGIC_REQUEST,
                                  GEARMAN_COMMAND_OPTION_REQ,
                                  args, args_size, 1);
  if (gearman_failed(ret))
  {
    gearman_error(&universal, GEARMAN_MEMORY_ALLOCATION_FAILURE, "gearman_packet_create_args()");
    return ret;
  }

  _push_blocking(&universal, &orig_block_universal);

  for (con= universal.con_list; con != NULL; con= con->next)
  {
    gearman_packet_st *packet_ptr;

    ret= con->send(&packet, true);
    if (gearman_failed(ret))
    {
      gearman_packet_free(&(con->packet));
      goto exit;
    }

    packet_ptr= con->recv(&(con->packet), &ret, true);
    if (gearman_failed(ret))
    {
      gearman_packet_free(&(con->packet));
      goto exit;
    }

    assert(packet_ptr);

    if (packet_ptr->command == GEARMAN_COMMAND_ERROR)
    {
      gearman_packet_free(&(con->packet));
      gearman_error(&universal, GEARMAN_INVALID_ARGUMENT, "invalid server option");

      ret= GEARMAN_INVALID_ARGUMENT;;
      goto exit;
    }

    gearman_packet_free(&(con->packet));
  }

  ret= GEARMAN_SUCCESS;

exit:
  gearman_packet_free(&packet);
  _pop_non_blocking(&universal, orig_block_universal);

  return gearman_success(ret);
}

void gearman_free_all_packets(gearman_universal_st &universal)
{
  while (universal.packet_list)
    gearman_packet_free(universal.packet_list);
}

/*
 * Local Definitions
 */

void gearman_universal_set_error(gearman_universal_st *universal, 
				 gearman_return_t rc,
				 const char *function,
                                 const char *format, ...)
{
  size_t size;
  char log_buffer[GEARMAN_MAX_ERROR_SIZE];
  char *ptr= log_buffer;
  va_list args;

  universal->error.rc= rc;
  if (rc != GEARMAN_ERRNO)
  {
    universal->error.last_errno= 0;
  }

  size= strlen(gearman_strerror(rc));
  ptr= static_cast<char *>(memcpy(ptr, gearman_strerror(rc), size));
  ptr+= size;

  ptr[0]= '-';
  size++;
  ptr++;

  ptr[0]= '>';
  size++;
  ptr++;

  size= strlen(function);
  ptr= static_cast<char *>(memcpy(ptr, static_cast<void*>(const_cast<char *>(function)), size));
  ptr+= size;

  ptr[0]= ':';
  size++;
  ptr++;

  ptr[0]= ' ';
  size++;
  ptr++;

  va_start(args, format);
  size+= size_t(vsnprintf(ptr, GEARMAN_MAX_ERROR_SIZE - size, format, args));
  va_end(args);

  if (universal->log_fn == NULL)
  {
    if (size >= GEARMAN_MAX_ERROR_SIZE)
      size= GEARMAN_MAX_ERROR_SIZE - 1;

    memcpy(universal->error.last_error, log_buffer, size + 1);
  }
  else
  {
    universal->log_fn(log_buffer, GEARMAN_VERBOSE_FATAL,
                      static_cast<void *>(universal->log_context));
  }
}

void gearman_universal_set_perror(const char *position, gearman_universal_st &self, const char *message)
{
  self.error.rc= GEARMAN_ERRNO;
  self.error.last_errno= errno;

  const char *errmsg_ptr;
  char errmsg[GEARMAN_MAX_ERROR_SIZE]; 
  errmsg[0]= 0; 

#ifdef STRERROR_R_CHAR_P
  errmsg_ptr= strerror_r(self.error.last_errno, errmsg, sizeof(errmsg));
#else
  strerror_r(self.error.last_errno, errmsg, sizeof(errmsg));
  errmsg_ptr= errmsg;
#endif

  char final[GEARMAN_MAX_ERROR_SIZE];
  snprintf(final, sizeof(final), "%s(%s)", message, errmsg_ptr);

  gearman_universal_set_error(&self, GEARMAN_ERRNO, position, final);
}
