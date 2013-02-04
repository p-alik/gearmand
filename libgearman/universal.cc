/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2011-2012 Data Differential, http://datadifferential.com/
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

#include "gear_config.h"
#include <libgearman/common.h>

#include "libgearman/assert.hpp"
#include "libgearman/interface/push.hpp"
#include "libgearman/server_options.hpp"

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <memory>

void gearman_nap(int arg)
{
  if (arg < 1)
  { }
  else
  {
#ifdef WIN32
    sleep(arg/1000000);
#else
    struct timespec global_sleep_value= { 0, static_cast<long>(arg * 1000)};
    nanosleep(&global_sleep_value, NULL);
#endif
  }
}

void gearman_nap(gearman_universal_st &self)
{
  gearman_nap(self.timeout);
}

void gearman_universal_clone(gearman_universal_st &destination, const gearman_universal_st &source, bool has_wakeup_fd)
{
  int wakeup_fd[2];

  if (has_wakeup_fd)
  {
    wakeup_fd[0]= destination.wakeup_fd[0];
    wakeup_fd[1]= destination.wakeup_fd[1];
  }

  if (has_wakeup_fd)
  {
    destination.wakeup_fd[0]= wakeup_fd[0];
    destination.wakeup_fd[1]= wakeup_fd[1];
  }

  (void)gearman_universal_set_option(destination, GEARMAN_NON_BLOCKING, source.options.non_blocking);

  destination.timeout= source.timeout;

  destination._namespace= gearman_string_clone(source._namespace);

  for (gearman_connection_st *con= source.con_list; con; con= con->next)
  {
    if (gearman_connection_copy(destination, *con) == NULL)
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
    universal.pfds= NULL;
  }

  // clean-up server options
  while (universal.server_options_list)
  {
    delete universal.server_options_list;
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

void gearman_set_workload_malloc_fn(gearman_universal_st& universal,
                                    gearman_malloc_fn *function,
                                    void *context)
{
  universal.allocator.malloc= function;
  universal.allocator.context= context;
}

void gearman_set_workload_free_fn(gearman_universal_st& universal,
                                  gearman_free_fn *function,
                                  void *context)
{
  universal.allocator.free= function;
  universal.allocator.context= context;
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
    con->close_socket();
  }
}

/*
 * Flush all shouldn't return any error, because there's no way to indicate
 * which connection experienced an issue. Error detection is better done in gearman_wait()
 * after flushing all the connections here.
 */
void gearman_flush_all(gearman_universal_st& universal)
{
  for (gearman_connection_st *con= universal.con_list; con; con= con->next)
  {
    if (con->events & POLLOUT)
    {
      continue;
    }

    con->flush();
  }
}

gearman_return_t gearman_wait(gearman_universal_st& universal)
{
  struct pollfd *pfds;

  bool have_shutdown_pipe= universal.wakeup_fd[0] == INVALID_SOCKET ? false : true;
  size_t con_count= universal.con_count +int(have_shutdown_pipe);

  if (universal.pfds_size < con_count)
  {
    pfds= static_cast<pollfd*>(realloc(universal.pfds, con_count * sizeof(struct pollfd)));
    if (pfds == NULL)
    {
      gearman_perror(universal, "pollfd realloc");
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;
    }

    universal.pfds= pfds;
    universal.pfds_size= int(con_count);
  }
  else
  {
    pfds= universal.pfds;
  }

  nfds_t x= 0;
  for (gearman_connection_st *con= universal.con_list; con; con= con->next)
  {
    if (con->events == 0)
    {
      continue;
    }

    pfds[x].fd= con->fd;
    pfds[x].events= con->events;
    pfds[x].revents= 0;
    x++;
  }

  if (x == 0)
  {
    return gearman_error(universal, GEARMAN_NO_ACTIVE_FDS, "no active file descriptors");
  }

  // Wakeup handling, we only make use of this if we have active connections
  size_t pipe_array_iterator= 0;
  if (have_shutdown_pipe)
  {
    pipe_array_iterator= x;
    pfds[x].fd= universal.wakeup_fd[0];
    pfds[x].events= POLLIN;
    pfds[x].revents= 0;
    x++;
  }

  int ret= 0;
  while (universal.timeout)
  {
    ret= poll(pfds, x, universal.timeout);
    if (ret == -1)
    {
      switch(errno)
      {
      case EINTR:
        continue;

      case EINVAL:
        return gearman_perror(universal, "RLIMIT_NOFILE exceeded, or if OSX the timeout value was invalid");

      default:
        return gearman_perror(universal, "poll");
      }
    }

    break;
  }

  if (ret == 0)
  {
    return gearman_universal_set_error(universal, GEARMAN_TIMEOUT, GEARMAN_AT,
                                       "timeout reached, %u servers were poll(), no servers were available, pipe:%s",
                                       uint32_t(x), have_shutdown_pipe ? "true" : "false");
  }

  x= 0;
  for (gearman_connection_st *con= universal.con_list; con; con= con->next)
  {
    if (con->events == 0)
    {
      continue;
    }

    if (pfds[x].revents & (POLLERR | POLLHUP | POLLNVAL))
    {
      int err;
      socklen_t len= sizeof (err);
      if (getsockopt(pfds[x].fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0)
      {
        con->cached_errno= err;
      }
    }

    con->set_revents(pfds[x].revents);

    x++;
  }

  if (have_shutdown_pipe and pfds[pipe_array_iterator].revents)
  {
    char buffer[1];
    ssize_t read_length= read(universal.wakeup_fd[0], buffer, sizeof(buffer));
    if (read_length > 0)
    {
      gearman_return_t local_ret= gearman_kill(gearman_universal_id(universal), GEARMAN_INTERRUPT);
      if (gearman_failed(local_ret))
      {
        return GEARMAN_SHUTDOWN;
      }

      return GEARMAN_SHUTDOWN_GRACEFUL;
    }

    if (read_length == 0)
    {
      return GEARMAN_SHUTDOWN;
    }

#if 0
    perror("shudown read");
#endif
    // @todo figure out what happens in an error
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

gearman_return_t gearman_set_identifier(gearman_universal_st& universal,
                                        const char *id, size_t id_size)
{
  if (id == NULL)
  {
    return gearman_error(universal, GEARMAN_INVALID_ARGUMENT, "id was NULL");
  }

  if (id_size == 0)
  {
    return gearman_error(universal, GEARMAN_INVALID_ARGUMENT,  "id_size was 0");
  }

  if (id_size > GEARMAN_MAX_IDENTIFIER)
  {
    return gearman_error(universal, GEARMAN_ARGUMENT_TOO_LARGE,  "id_size was greater then GEARMAN_MAX_ECHO_SIZE");
  }

  for (size_t x= 0; x < id_size; x++)
  {
    if (isgraph(id[x]) == false)
    {
      return gearman_error(universal, GEARMAN_INVALID_ARGUMENT,  "Invalid character found in identifier");
    }
  }

  gearman_packet_st packet;
  gearman_return_t ret= gearman_packet_create_args(universal, packet, GEARMAN_MAGIC_REQUEST,
                                                   GEARMAN_COMMAND_SET_CLIENT_ID,
                                                   (const void**)&id, &id_size, 1);
  if (gearman_success(ret))
  {
    PUSH_BLOCKING(universal);

    for (gearman_connection_st *con= universal.con_list; con; con= con->next)
    {
      gearman_return_t local_ret= con->send_packet(packet, true);
      if (gearman_failed(local_ret))
      {
        ret= local_ret;
      }

    }
  }

  gearman_packet_free(&packet);

  return ret;
}

EchoCheck::EchoCheck(gearman_universal_st& universal_,
    const void *workload_, const size_t workload_size_) :
    _universal(universal_),
    _workload(workload_),
    _workload_size(workload_size_)
{
}

gearman_return_t EchoCheck::success(gearman_connection_st* con)
{
  if (con->_packet.command != GEARMAN_COMMAND_ECHO_RES)
  {
    return gearman_error(_universal, GEARMAN_INVALID_COMMAND, "Wrong command sent in response to ECHO request");
  }

  if (con->_packet.data_size != _workload_size or
      memcmp(_workload, con->_packet.data, _workload_size))
  {
    return gearman_error(_universal, GEARMAN_ECHO_DATA_CORRUPTION, "corruption during echo");
  }

  return GEARMAN_SUCCESS;
}

OptionCheck::OptionCheck(gearman_universal_st& universal_):
    _universal(universal_)
{
}

gearman_return_t OptionCheck::success(gearman_connection_st* con)
{
  if (con->_packet.command == GEARMAN_COMMAND_ERROR)
  {
    return gearman_error(_universal, GEARMAN_INVALID_SERVER_OPTION, "invalid server option");
  }

  return GEARMAN_SUCCESS;
}

static gearman_return_t connection_loop(gearman_universal_st& universal,
                                        const gearman_packet_st& message,
                                        Check& check)
{
  gearman_return_t ret= GEARMAN_SUCCESS;

  for (gearman_connection_st *con= universal.con_list; con; con= con->next)
  {
    ret= con->send_packet(message, true);
    if (gearman_failed(ret))
    {
#if 0
      assert_msg(con->universal.error.rc != GEARMAN_SUCCESS, "Programmer error, error returned but not recorded");
#endif
      break;
    }

    con->options.packet_in_use= true;
    gearman_packet_st *packet_ptr= con->receiving(con->_packet, ret, true);
    if (packet_ptr == NULL)
    {
      assert(&con->_packet == universal.packet_list);
      con->options.packet_in_use= false;
      break;
    }

    assert(packet_ptr == &con->_packet);
    if (gearman_failed(ret))
    {
#if 0
      assert_msg(con->universal.error.rc != GEARMAN_SUCCESS, "Programmer error, error returned but not recorded");
#endif
      con->free_private_packet();
      con->reset_recv_packet();

      break;
    }
    assert(packet_ptr);

    if (gearman_failed(ret= check.success(con)))
    {
#if 0
      assert_msg(con->universal.error.rc != GEARMAN_SUCCESS, "Programmer error, error returned but not recorded");
#endif
      con->free_private_packet();
      con->reset_recv_packet();

      break;
    }

    con->reset_recv_packet();
    con->free_private_packet();
  }

  return ret;
}


gearman_return_t gearman_echo(gearman_universal_st& universal,
                              const void *workload,
                              size_t workload_size)
{
  if (workload == NULL)
  {
    return gearman_error(universal, GEARMAN_INVALID_ARGUMENT, "workload was NULL");
  }

  if (workload_size == 0)
  {
    return gearman_error(universal, GEARMAN_INVALID_ARGUMENT,  "workload_size was 0");
  }

  if (workload_size > GEARMAN_MAX_ECHO_SIZE)
  {
    return gearman_error(universal, GEARMAN_ARGUMENT_TOO_LARGE,  "workload_size was greater then GEARMAN_MAX_ECHO_SIZE");
  }

  gearman_packet_st message;
  gearman_return_t ret= gearman_packet_create_args(universal, message, GEARMAN_MAGIC_REQUEST,
                                                   GEARMAN_COMMAND_ECHO_REQ,
                                                   &workload, &workload_size, 1);
  if (gearman_success(ret))
  {
    PUSH_BLOCKING(universal);

    EchoCheck check(universal, workload, workload_size);
    ret= connection_loop(universal, message, check);
  }
  else
  {
    gearman_packet_free(&message);
    gearman_error(universal, GEARMAN_MEMORY_ALLOCATION_FAILURE, "gearman_packet_create_args()");
    return ret;
  }

  gearman_packet_free(&message);

  return ret;
}

bool gearman_request_option(gearman_universal_st &universal,
                            gearman_string_t &option)
{
  char* option_str_cpy = (char*) malloc(gearman_size(option));

  if (option_str_cpy == NULL)
  {
    gearman_error(universal, GEARMAN_MEMORY_ALLOCATION_FAILURE, "malloc()");
    return false;
  }

  strncpy(option_str_cpy, gearman_c_str(option), gearman_size(option));

  gearman_server_options_st *server_options = new (std::nothrow) gearman_server_options_st(universal, option_str_cpy, gearman_size(option));
  if (server_options == NULL)
  {
    free(option_str_cpy);
    gearman_error(universal, GEARMAN_MEMORY_ALLOCATION_FAILURE, "new gearman_server_options_st()");
    return false;
  }

  return true;
}

void gearman_free_all_packets(gearman_universal_st &universal)
{
  while (universal.packet_list)
  {
    gearman_packet_free(universal.packet_list);
  }
}

gearman_id_t gearman_universal_id(gearman_universal_st &universal)
{
  gearman_id_t handle= { universal.wakeup_fd[0], universal.wakeup_fd[1] };

  return handle;
}

/*
 * Local Definitions
 */

void gearman_universal_set_namespace(gearman_universal_st& universal, const char *namespace_key, size_t namespace_key_size)
{
  gearman_string_free(universal._namespace);
  universal._namespace= gearman_string_create(NULL, namespace_key, namespace_key_size);
}

const char *gearman_univeral_namespace(gearman_universal_st& universal)
{
  return gearman_string_value(universal._namespace);
}
