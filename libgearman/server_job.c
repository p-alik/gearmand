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
                                              const void *data,
                                              size_t data_size,
                                              gearman_server_con_st *server_con,
                                              bool high)
{
  gearman_server_job_st *server_job;

  server_job= gearman_server_job_create(server, NULL);
  if (server_job == NULL)
    return NULL;

  snprintf(server_job->job_handle, GEARMAN_JOB_HANDLE_SIZE, "%s:%u",
           server->job_handle_prefix, server->job_handle_count);
  server->job_handle_count++;
  server_job->data= data;
  server_job->data_size= data_size;
  server_job->client= server_con;

  (void) high;
  /* Queue job, possibly as high priority */
  (void) function_name;
  /* Add function if needed, function_get can automatically create. */
  /* Queue NOOP for idle workers of function. */

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
