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

#include <config.h>
#include <libgearman-server/error.h>
#include <libgearman-server/constants.h>
#include <libgearman-server/connection_list.h>
#include <libgearman-server/io.h>
#include <libgearman-server/connection.h>
#include <assert.h>

void gearmand_connection_list_init(gearmand_connection_list_st *universal,
                                   gearmand_event_watch_fn *watch_fn, void *watch_context)
{
  assert(universal);

  universal->con_count= 0;
  universal->con_list= NULL;
  universal->event_watch_fn= watch_fn;
  universal->event_watch_context= watch_context;
}

void gearman_connection_list_free(gearmand_connection_list_st *universal)
{
  while (universal->con_list)
    gearmand_io_free(universal->con_list);
}

gearman_server_con_st *gearmand_ready(gearmand_connection_list_st *universal)
{
  /* We can't keep universal between calls since connections may be removed during
    processing. If this list ever gets big, we may want something faster. */

  for (gearmand_io_st *con= universal->con_list; con; con= con->next)
  {
    if (con->options.ready)
    {
      con->options.ready= false;
      return con->root;
    }
  }

  return NULL;
}
