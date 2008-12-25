/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server Connection definitions
 */

#include "common.h"

/*
 * Public definitions
 */

gearman_server_con_st *gearman_server_con_create(gearman_server_st *server,
                                             gearman_server_con_st *server_con)
{
  if (server_con == NULL)
  {
    server_con= malloc(sizeof(gearman_server_con_st));
    if (server_con == NULL)
    {
      GEARMAN_ERROR_SET(server->gearman, "gearman_server_con_create:malloc");
      return NULL;
    }

    memset(server_con, 0, sizeof(gearman_server_con_st));
    server_con->options|= GEARMAN_SERVER_CON_ALLOCATED;
  }
  else
    memset(server_con, 0, sizeof(gearman_server_con_st));

  if (gearman_con_create(server->gearman, &(server_con->con)) == NULL)
  {
    if (server_con->options & GEARMAN_SERVER_CON_ALLOCATED)
      free(server_con);

    return NULL;
  }

  server_con->server= server;

  if (server->server_con_list)
    server->server_con_list->prev= server_con;
  server_con->next= server->server_con_list;
  server->server_con_list= server_con;
  server->server_con_count++;

  return server_con;
}

gearman_return_t gearman_server_con_free(gearman_server_con_st *server_con)
{
  gearman_con_free(&(server_con->con));

  if (server_con->server->server_con_list == server_con)
    server_con->server->server_con_list= server_con->next;
  if (server_con->prev)
    server_con->prev->next= server_con->next;
  if (server_con->next)
    server_con->next->prev= server_con->prev;
  server_con->server->server_con_count--;

  if (server_con->options & GEARMAN_SERVER_CON_ALLOCATED)
    free(server_con);

  return GEARMAN_SUCCESS;
}

void *gearman_server_con_data(gearman_server_con_st *server_con)
{
  return gearman_con_data(&(server_con->con));
}
  
void gearman_server_con_set_data(gearman_server_con_st *server_con, void *data)
{
  gearman_con_set_data(&(server_con->con), data);
}
