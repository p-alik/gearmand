/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#pragma once 

struct gearmand_io_st;

#ifdef __cplusplus
extern "C" {
#endif

struct gearmand_connection_list_st
{
  uint32_t con_count;
  gearmand_io_st *con_list;
  gearmand_event_watch_fn *event_watch_fn; // Function to be called when events need to be watched
  void *event_watch_context;
};

typedef struct gearmand_connection_list_st gearmand_connection_list_st;

GEARMAN_INTERNAL_API
void gearmand_connection_list_init(gearmand_connection_list_st *source, gearmand_event_watch_fn *watch_fn, void *watch_context);

GEARMAN_INTERNAL_API
void gearman_connection_list_free(gearmand_connection_list_st *gearman);

/*
  Get next connection that is ready for I/O.
*/
GEARMAN_INTERNAL_API
gearman_server_con_st *gearmand_ready(gearmand_connection_list_st *gearman);

#ifdef __cplusplus
}
#endif
