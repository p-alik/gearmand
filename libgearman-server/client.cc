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

#include <config.h>
#include <libgearman-server/common.h>

#include <cassert>
#include <memory>

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
    client= new (std::nothrow) gearman_server_client_st;
    if (client == NULL)
    {
      gearmand_merror("new", gearman_server_client_st,  0);
      return NULL;
    }
  }
  assert(client);

  if (client == NULL)
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
  if (client == NULL)
  {
    return;
  }

  GEARMAN_LIST_DEL(client->con->client, client, con_)

  if (client->job)
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
    gearmand_debug("delete gearman_server_client_st");
    delete client;
  }
}
