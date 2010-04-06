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
gearman_server_client_add(gearman_server_con_st *con)
{
  gearman_server_client_st *client;

  client= gearman_server_client_create(con, NULL);
  if (client == NULL)
    return NULL;

  return client;
}

gearman_server_client_st *
gearman_server_client_create(gearman_server_con_st *con,
                             gearman_server_client_st *client)
{
  gearman_server_st *server= con->thread->server;

  if (client == NULL)
  {
    if (server->free_client_count > 0)
    {
      client= server->free_client_list;
      GEARMAN_LIST_DEL(server->free_client, client, con_)
    }
    else
    {
      client= (gearman_server_client_st *)malloc(sizeof(gearman_server_client_st));
      if (client == NULL)
      {
        gearman_log_error(con->thread->gearman, "gearman_server_client_create", "malloc");
        return NULL;
      }
    }

    client->options.allocated= true;
  }
  else
  {
    client->options.allocated= false;
  }

  client->con= con;
  GEARMAN_LIST_ADD(con->client, client, con_)
  client->job= NULL;
  client->job_next= NULL;
  client->job_prev= NULL;

  return client;
}

void gearman_server_client_free(gearman_server_client_st *client)
{
  gearman_server_st *server= client->con->thread->server;

  GEARMAN_LIST_DEL(client->con->client, client, con_)

  if (client->job != NULL)
  {
    GEARMAN_LIST_DEL(client->job->client, client, job_)

    /* If this was a foreground job and is now abandoned, mark to not run. */
    if (client->job->client_list == NULL)
    {
      client->job->ignore_job= true;
      client->job->job_queued= false;
    }
  }

  if (client->options.allocated)
  {
    if (server->free_client_count < GEARMAN_MAX_FREE_SERVER_CLIENT)
      GEARMAN_LIST_ADD(server->free_client, client, con_)
    else
      free(client);
  }
}
