/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server job definitions
 */

#include "common.h"

/*
 * Public definitions
 */

gearman_server_job_st *gearman_server_job_add(gearman_server_st *server,
                                              const char *function_name,
                                              size_t function_name_size,
                                              const void *data,
                                              size_t data_size,
                                              gearman_server_con_st *server_con,
                                              bool high)
{
  gearman_server_function_st *server_function;
  gearman_server_job_st *server_job;
  gearman_server_worker_st *server_worker;
  gearman_server_packet_st *server_packet;
  gearman_return_t ret;

  server_function= gearman_server_function_get(server, function_name,
                                               function_name_size);
  if (server_function == NULL)
    return NULL;

  server_job= gearman_server_job_create(server, NULL);
  if (server_job == NULL)
    return NULL;

  /* Queue NOOP for possible sleeping workers. */
  /* TODO Move this into it's own function or add ret_ptr arg. */
  for (server_worker= server_function->worker_list; server_worker != NULL;
       server_worker= server_worker->function_next)
  {
    if (!(server_worker->con->options & GEARMAN_SERVER_CON_SLEEPING))
      continue;

    server_packet= gearman_server_con_packet_add(server_worker->con);
    if (server_packet == NULL)
    {
      gearman_server_job_free(server_job);
      return NULL;
    }

    ret= gearman_packet_add(server->gearman, &(server_packet->packet),
                            GEARMAN_MAGIC_RESPONSE, GEARMAN_COMMAND_NOOP, NULL);
    if (ret != GEARMAN_SUCCESS)
    {
      gearman_server_job_free(server_job);
      return NULL;
    }

    server_packet->options|= GEARMAN_SERVER_PACKET_IN_USE;
    gearman_server_active_list_add(server_worker->con);
  }

  snprintf(server_job->job_handle, GEARMAN_JOB_HANDLE_SIZE, "%s:%u",
           server->job_handle_prefix, server->job_handle_count);
  server->job_handle_count++;
  server_job->data= data;
  server_job->data_size= data_size;
  server_job->client= server_con;

  server_job->function= server_function;

  /* Queue the job to be run. */
  if (high)
  {
    if (server_function->job_high_end == NULL)
    {
      if (server_function->job_list != NULL)
        server_job->function_next= server_function->job_list;
      server_function->job_list= server_job;
    }
    else
    {
      server_job->function_next= server_function->job_high_end->function_next;
      server_function->job_high_end->function_next= server_job;
    }
    server_function->job_high_end= server_job;
  }
  else
  {
    if (server_function->job_end == NULL)
    {
      if (server_function->job_list == NULL)
        server_function->job_list= server_job;
      else
        server_function->job_high_end->function_next= server_job;
    }
    else
      server_job->function_next= server_function->job_end->function_next;
    server_function->job_end= server_job;
  }

  return server_job;
}

gearman_server_job_st *gearman_server_job_create(gearman_server_st *server,
                                             gearman_server_job_st *server_job)
{
  if (server_job == NULL)
  {
    server_job= malloc(sizeof(gearman_server_job_st));
    if (server_job == NULL)
    {
      GEARMAN_ERROR_SET(server->gearman, "gearman_server_job_create", "malloc")
      return NULL;
    }

    memset(server_job, 0, sizeof(gearman_server_job_st));
    server_job->options|= GEARMAN_SERVER_JOB_ALLOCATED;
  }
  else
    memset(server_job, 0, sizeof(gearman_server_job_st));

  server_job->server= server;

  if (server->job_list)
    server->job_list->prev= server_job;
  server_job->next= server->job_list;
  server->job_list= server_job;
  server->job_count++;

  return server_job;
}

void gearman_server_job_free(gearman_server_job_st *server_job)
{
  if (server_job->data != NULL)
    free((void *)(server_job->data));

  if (server_job->server->job_list == server_job)
    server_job->server->job_list= server_job->next;
  if (server_job->prev)
    server_job->prev->next= server_job->next;
  if (server_job->next)
    server_job->next->prev= server_job->prev;
  server_job->server->job_count--;

  if (server_job->options & GEARMAN_SERVER_JOB_ALLOCATED)
    free(server_job);
}

gearman_server_job_st *gearman_server_job_get(gearman_server_st *server,
                                              gearman_job_handle_t job_handle)
{
  gearman_server_job_st *server_job;

  for (server_job= server->job_list; server_job != NULL;
       server_job= server_job->next)
  {
    if (!strcmp(server_job->job_handle, job_handle))
      return server_job;
  }

  return NULL;
}

gearman_server_job_st *gearman_server_job_get_new(gearman_server_con_st *server_con)
{
  gearman_server_worker_st *server_worker;

  for (server_worker= server_con->worker_list; server_worker != NULL;
       server_worker= server_worker->con_next)
  {
    if (server_worker->function->job_list != NULL)
      return server_worker->function->job_list;
  }

  return NULL;
}

void gearman_server_job_run(gearman_server_job_st *server_job,
                            gearman_server_con_st *server_con)
{
  assert(server_job == server_job->function->job_list);

  server_job->worker= server_con;

  server_job->function->job_list= server_job->function_next;
  if (server_job->function->job_end == server_job)
    server_job->function->job_end= NULL;
  else if (server_job->function->job_high_end == server_job)
    server_job->function->job_high_end= NULL;
  server_job->function_next= NULL;
}
