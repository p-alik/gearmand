/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server client definitions
 */

#include "common.h"

/*
 * Public definitions
 */

gearman_server_client_st *
gearman_server_client_add(gearman_server_con_st *server_con)
{
  gearman_server_client_st *server_client;

  server_client= gearman_server_client_create(server_con, NULL);
  if (server_client == NULL)
    return NULL;

  return server_client;
}

gearman_server_client_st *
gearman_server_client_create(gearman_server_con_st *server_con,
                             gearman_server_client_st *server_client)
{
  if (server_client == NULL)
  {
    server_client= malloc(sizeof(gearman_server_client_st));
    if (server_client == NULL)
    {
      GEARMAN_ERROR_SET(server_con->server->gearman,
                        "gearman_server_client_create", "malloc")
      return NULL;
    }

    memset(server_client, 0, sizeof(gearman_server_client_st));
    server_client->options|= GEARMAN_SERVER_CLIENT_ALLOCATED;
  }
  else
    memset(server_client, 0, sizeof(gearman_server_client_st));

  server_client->con= server_con;

  GEARMAN_LIST_ADD(server_con->client, server_client, con_)

  return server_client;
}

void gearman_server_client_free(gearman_server_client_st *server_client)
{
  GEARMAN_LIST_DEL(server_client->con->client, server_client, con_)

  if (server_client->job != NULL)
    GEARMAN_LIST_DEL(server_client->job->client, server_client, job_)

  if (server_client->options & GEARMAN_SERVER_CLIENT_ALLOCATED)
    free(server_client);
}
