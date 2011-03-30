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

#include <libgearman-server/common.h>
#include <assert.h>

/*
 * Public definitions
 */

gearman_server_client_st *
gearman_server_client_add(gearman_server_con_st *con)
{
  gearman_server_client_st *client;

  if (Server->free_client_count > 0)
  {
    client= Server->free_client_list;
    GEARMAN_LIST_DEL(Server->free_client, client, con_)
  }
  else
  {
    client= static_cast<gearman_server_client_st *>(malloc(sizeof(gearman_server_client_st)));
    if (client == NULL)
    {
      gearmand_log_error("gearman_server_client_create", "malloc");
      return NULL;
    }
  }
  assert(client);

  if (!client)
  {
    gearmand_error("In gearman_server_client_add() we failed to either allocorate of find a free one");
    return NULL;
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

  if (Server->free_client_count < GEARMAN_MAX_FREE_SERVER_CLIENT)
  {
    GEARMAN_LIST_ADD(Server->free_client, client, con_)
  }
  else
  {
    gearmand_crazy("free");
    free(client);
  }
}
