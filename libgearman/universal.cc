/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2008 Brian Aker, Eric Day
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


/**
 * @file
 * @brief Gearman State Definitions
 */

#include <libgearman/common.h>
#include <libgearman/connection.h>
#include <libgearman/packet.hpp>
#include <libgearman/universal.hpp>
#include <libgearman/vector.hpp>

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
  self._namespace= NULL;
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

  destination._namespace= gearman_string_clone(source._namespace);

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
  gearman_free_all_cons(universal);
  gearman_free_all_packets(universal);
  gearman_string_free(universal._namespace);

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

void *gearman_real_malloc(gearman_universal_st& universal, size_t size, const char *func, const char *file, int line)
{
  void *ptr;
  if (universal.workload_malloc_fn)
  {
    ptr= universal.workload_malloc_fn(size, universal.workload_malloc_context);
  }
  else
  {
    ptr= malloc(size);
  }

#if 0
  fprintf(stderr, "gearman_real_malloc(%s, %lu) : %p -> %s:%d\n", func, static_cast<unsigned long>(size), ptr,  file, line);
#else
  (void)func; (void)file; (void)line;
#endif


  return ptr;
}

void gearman_real_free(gearman_universal_st& universal, void *ptr, const char *func, const char *file, int line)
{
#if 0
  fprintf(stderr, "gearman_real_free(%s) : %p -> %s:%d\n", func, ptr, file, line);
#else
  (void)func; (void)file; (void)line;
#endif

  if (universal.workload_free_fn)
  {
    universal.workload_free_fn(ptr, universal.workload_free_context);
  }
  else
  {
    free(ptr);
  }
}

void gearman_set_workload_malloc_fn(gearman_universal_st& universal,
                                    gearman_malloc_fn *function,
                                    void *context)
{
  universal.workload_malloc_fn= function;
  universal.workload_malloc_context= context;
}

void gearman_set_workload_free_fn(gearman_universal_st& universal,
                                  gearman_free_fn *function,
                                  void *context)
{
  universal.workload_free_fn= function;
  universal.workload_free_context= context;
}

void gearman_free_all_cons(gearman_universal_st& universal)
{
  while (universal.con_list)
  {
    delete universal.con_list;
  }
}

void gearman_reset(gearman_universal_st& universal)
{
  for (gearman_connection_st *con= universal.con_list; con; con= con->next)
  {
    con->close();
  }
}

gearman_return_t gearman_flush_all(gearman_universal_st& universal)
{
  for (gearman_connection_st *con= universal.con_list; con; con= con->next)
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
    if (not pfds)
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
  for (gearman_connection_st *con= universal.con_list; con; con= con->next)
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
    return gearman_error(universal, GEARMAN_NO_ACTIVE_FDS, "no active file descriptors");
  }

  int ret= 0;
  while (universal.timeout)
  {
    ret= poll(pfds, x, universal.timeout);
    if (ret == -1)
    {
      if (errno == EINTR)
        continue;

      return gearman_perror(universal, "poll");
    }

    break;
  }

  if (not ret)
  {
    gearman_error(universal, GEARMAN_TIMEOUT, "timeout reached");
    return GEARMAN_TIMEOUT;
  }

  x= 0;
  for (gearman_connection_st *con= universal.con_list; con; con= con->next)
  {
    if (con->events == 0)
      continue;

    int err;
    socklen_t len= sizeof (err);
    (void)getsockopt(con->fd, SOL_SOCKET, SO_ERROR, &err, &len);
    if (err)
    {
      con->cached_errno= err;
    }

    con->set_revents(pfds[x].revents);

    x++;
  }

  return GEARMAN_SUCCESS;
}

gearman_connection_st *gearman_ready(gearman_universal_st& universal)
{
  /* 
    We can't keep universal between calls since connections may be removed during
    processing. If this list ever gets big, we may want something faster.
  */
  for (gearman_connection_st *con= universal.con_list; con; con= con->next)
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
static inline void _push_blocking(gearman_universal_st& universal, bool &orig_block_universal)
{
  orig_block_universal= universal.options.non_blocking;
  universal.options.non_blocking= false;
}

static inline void _pop_non_blocking(gearman_universal_st& universal, bool orig_block_universal)
{
  universal.options.non_blocking= orig_block_universal;
}

gearman_return_t gearman_echo(gearman_universal_st& universal,
                              const void *workload,
                              size_t workload_size)
{
  gearman_packet_st packet;
  bool orig_block_universal;

  if (not workload)
  {
    gearman_error(universal, GEARMAN_INVALID_ARGUMENT, "workload was NULL");
    return GEARMAN_INVALID_ARGUMENT;
  }

  if (not workload_size)
  {
    gearman_error(universal, GEARMAN_INVALID_ARGUMENT,  "workload_size was 0");
    return GEARMAN_INVALID_ARGUMENT;
  }

  gearman_return_t ret= gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                   GEARMAN_COMMAND_ECHO_REQ,
                                                   &workload, &workload_size, 1);
  if (gearman_failed(ret))
  {
    return ret;
  }

  _push_blocking(universal, orig_block_universal);

  for (gearman_connection_st *con= universal.con_list; con; con= con->next)
  {
    gearman_packet_st *packet_ptr;

    ret= con->send(packet, true);
    if (gearman_failed(ret))
    {
      goto exit;
    }

    packet_ptr= con->receiving(con->_packet, ret, true);
    if (gearman_failed(ret))
    {
      goto exit;
    }

    assert(packet_ptr);

    if (con->_packet.data_size != workload_size ||
        memcmp(workload, con->_packet.data, workload_size))
    {
      gearman_packet_free(&(con->_packet));
      gearman_error(universal, GEARMAN_ECHO_DATA_CORRUPTION, "corruption during echo");

      ret= GEARMAN_ECHO_DATA_CORRUPTION;
      goto exit;
    }

    gearman_packet_free(&(con->_packet));
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
  bool orig_block_universal;

  const void *args[]= { gearman_c_str(option) };
  size_t args_size[]= { gearman_size(option) };

  gearman_return_t ret;
  ret= gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                  GEARMAN_COMMAND_OPTION_REQ,
                                  args, args_size, 1);
  if (gearman_failed(ret))
  {
    gearman_error(universal, GEARMAN_MEMORY_ALLOCATION_FAILURE, "gearman_packet_create_args()");
    return ret;
  }

  _push_blocking(universal, orig_block_universal);

  for (con= universal.con_list; con != NULL; con= con->next)
  {
    gearman_packet_st *packet_ptr;

    ret= con->send(packet, true);
    if (gearman_failed(ret))
    {
      gearman_packet_free(&(con->_packet));
      goto exit;
    }

    packet_ptr= con->receiving(con->_packet, ret, true);
    if (gearman_failed(ret))
    {
      gearman_packet_free(&(con->_packet));
      goto exit;
    }

    assert(packet_ptr);

    if (packet_ptr->command == GEARMAN_COMMAND_ERROR)
    {
      gearman_packet_free(&(con->_packet));
      gearman_error(universal, GEARMAN_INVALID_ARGUMENT, "invalid server option");

      ret= GEARMAN_INVALID_ARGUMENT;;
      goto exit;
    }

    gearman_packet_free(&(con->_packet));
  }

  ret= GEARMAN_SUCCESS;

exit:
  gearman_packet_free(&packet);
  _pop_non_blocking(universal, orig_block_universal);

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

gearman_return_t gearman_universal_set_error(gearman_universal_st& universal, 
                                             gearman_return_t rc,
                                             const char *function,
                                             const char *position,
                                             const char *format, ...)
{
  va_list args;

  universal.error.rc= rc;
  if (rc != GEARMAN_ERRNO)
  {
    universal.error.last_errno= 0;
  }

  char last_error[GEARMAN_MAX_ERROR_SIZE];
  va_start(args, format);
  int length= vsnprintf(last_error, GEARMAN_MAX_ERROR_SIZE, format, args);
  va_end(args);

  if (length > int(GEARMAN_MAX_ERROR_SIZE) or length < 0)
  {
    assert(length > int(GEARMAN_MAX_ERROR_SIZE));
    assert(length < 0);
    universal.error.last_error[GEARMAN_MAX_ERROR_SIZE -1]= 0;
    return GEARMAN_ARGUMENT_TOO_LARGE;
  }

  length= snprintf(universal.error.last_error, GEARMAN_MAX_ERROR_SIZE, "%s(%s) %s -> %s", function, gearman_strerror(rc), last_error, position);
  if (length > int(GEARMAN_MAX_ERROR_SIZE) or length < 0)
  {
    assert(length > int(GEARMAN_MAX_ERROR_SIZE));
    assert(length < 0);
    universal.error.last_error[GEARMAN_MAX_ERROR_SIZE -1]= 0;
    return GEARMAN_ARGUMENT_TOO_LARGE;
  }

  if (universal.log_fn)
  {
    universal.log_fn(universal.error.last_error, GEARMAN_VERBOSE_FATAL,
                     static_cast<void *>(universal.log_context));
  }

  return rc;
}

gearman_return_t gearman_universal_set_gerror(gearman_universal_st& universal, 
                                              gearman_return_t rc,
                                              const char *func,
                                              const char *position)
{
  universal.error.rc= rc;
  if (rc != GEARMAN_ERRNO)
  {
    universal.error.last_errno= 0;
  }

  int length= snprintf(universal.error.last_error, GEARMAN_MAX_ERROR_SIZE, "%s(%s) -> %s", func, gearman_strerror(rc), position);
  if (length > int(GEARMAN_MAX_ERROR_SIZE) or length < 0)
  {
    assert(length > int(GEARMAN_MAX_ERROR_SIZE));
    assert(length < 0);
    universal.error.last_error[GEARMAN_MAX_ERROR_SIZE -1]= 0;
    return GEARMAN_ARGUMENT_TOO_LARGE;
  }

  if (universal.log_fn)
  {
    universal.log_fn(universal.error.last_error, GEARMAN_VERBOSE_FATAL,
                     static_cast<void *>(universal.log_context));
  }

  return rc;
}

gearman_return_t gearman_universal_set_perror(gearman_universal_st &universal,
                                              const char *function, const char *position, 
                                              const char *message)
{
  universal.error.rc= GEARMAN_ERRNO;
  universal.error.last_errno= errno;

  const char *errmsg_ptr;
  char errmsg[GEARMAN_MAX_ERROR_SIZE]; 
  errmsg[0]= 0; 

#ifdef STRERROR_R_CHAR_P
  errmsg_ptr= strerror_r(universal.error.last_errno, errmsg, sizeof(errmsg));
#else
  strerror_r(universal.error.last_errno, errmsg, sizeof(errmsg));
  errmsg_ptr= errmsg;
#endif

  int length;
  if (message)
  {
    length= snprintf(universal.error.last_error, GEARMAN_MAX_ERROR_SIZE, "%s(%s) %s -> %s", function, errmsg_ptr, message, position);
  }
  else
  {
    length= snprintf(universal.error.last_error, GEARMAN_MAX_ERROR_SIZE, "%s(%s) -> %s", function, errmsg_ptr, position);
  }

  if (length > int(GEARMAN_MAX_ERROR_SIZE) or length < 0)
  {
    assert(length > int(GEARMAN_MAX_ERROR_SIZE));
    assert(length < 0);
    universal.error.last_error[GEARMAN_MAX_ERROR_SIZE -1]= 0;
    return GEARMAN_ARGUMENT_TOO_LARGE;
  }

  if (universal.log_fn)
  {
    universal.log_fn(universal.error.last_error, GEARMAN_VERBOSE_FATAL,
                     static_cast<void *>(universal.log_context));
  }

  return GEARMAN_ERRNO;
}

void gearman_universal_set_namespace(gearman_universal_st& universal, const char *namespace_key, size_t namespace_key_size)
{
  gearman_string_free(universal._namespace);
  universal._namespace= gearman_string_create(NULL, namespace_key, namespace_key_size);
}
