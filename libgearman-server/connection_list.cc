/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
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
