/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2012 Data Differential, http://datadifferential.com/
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

#include <config.h>
#include <libgearman-server/common.h>

#include <cassert>
#include <memory>

gearmand_con_st* build_gearmand_con_st(void)
{
  return new (std::nothrow) gearmand_con_st;
}

void destroy_gearmand_con_st(gearmand_con_st* arg)
{
  gearmand_debug("delete gearmand_con_st");
  delete arg;
}

void _con_ready(int, short events, void *arg)
{
  gearmand_con_st *dcon= (gearmand_con_st *)(arg);
  short revents= 0;

  if (events & EV_READ)
  {
    revents|= POLLIN;
  }
  if (events & EV_WRITE)
  {
    revents|= POLLOUT;
  }

  gearmand_error_t ret= gearmand_io_set_revents(dcon->server_con, revents);
  if (gearmand_failed(ret))
  {
    gearmand_gerror("gearmand_io_set_revents", ret);
    gearmand_con_free(dcon);
    return;
  }

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM, 
                     "%s:%s Ready     %6s %s",
                     dcon->host, dcon->port,
                     revents & POLLIN ? "POLLIN" : "",
                     revents & POLLOUT ? "POLLOUT" : "");

  gearmand_thread_run(dcon->thread);
}

gearmand_error_t gearmand_connection_watch(gearmand_io_st *con, short events, void *)
{
  short set_events= 0;

  gearmand_con_st* dcon= gearman_io_context(con);

  if (events & POLLIN)
  {
    set_events|= EV_READ;
  }
  if (events & POLLOUT)
  {
    set_events|= EV_WRITE;
  }

  if (dcon->last_events != set_events)
  {
    if (dcon->last_events)
    {
      if (event_del(&(dcon->event)) < 0)
      {
        gearmand_perror("event_del");
        assert(! "event_del");
      }
    }
    event_set(&(dcon->event), dcon->fd, set_events | EV_PERSIST, _con_ready, dcon);
    event_base_set(dcon->thread->base, &(dcon->event));

    if (event_add(&(dcon->event), NULL) < 0)
    {
      gearmand_perror("event_add");
      return GEARMAN_EVENT;
    }

    dcon->last_events= set_events;
  }

  gearmand_log_debug(GEARMAN_DEFAULT_LOG_PARAM,
                     "%15s:%5s Watching  %6s %s",
                     dcon->host, dcon->port,
                     events & POLLIN ? "POLLIN" : "",
                     events & POLLOUT ? "POLLOUT" : "");

  return GEARMAN_SUCCESS;
}
