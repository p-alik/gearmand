/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/**
 * @file
 * @brief Server worker definitions
 */

#include "common.h"

/*
 * Public definitions
 */

gearman_server_worker_st *
gearman_server_worker_add(gearman_server_con_st *server_con,
                          const char *function_name,
                          size_t function_name_size,
                          uint32_t timeout)
{
  gearman_server_worker_st *server_worker;
  gearman_server_function_st *server_function;

  server_function= gearman_server_function_get(server_con->server,
                                               function_name,
                                               function_name_size);
  if (server_function == NULL)
    return NULL;

  server_worker= gearman_server_worker_create(server_con, server_function,
                                              NULL);
  if (server_worker == NULL)
    return NULL;

  server_worker->timeout= timeout;

  return server_worker;
}

gearman_server_worker_st *
gearman_server_worker_create(gearman_server_con_st *server_con,
                             gearman_server_function_st *server_function,
                             gearman_server_worker_st *server_worker)
{
  if (server_worker == NULL)
  {
    server_worker= malloc(sizeof(gearman_server_worker_st));
    if (server_worker == NULL)
    {
      GEARMAN_ERROR_SET(server_con->server->gearman,
                        "gearman_server_worker_create", "malloc")
      return NULL;
    }

    memset(server_worker, 0, sizeof(gearman_server_worker_st));
    server_worker->options|= GEARMAN_SERVER_WORKER_ALLOCATED;
  }
  else
    memset(server_worker, 0, sizeof(gearman_server_worker_st));

  server_worker->con= server_con;
  server_worker->function= server_function;

  GEARMAN_LIST_ADD(server_con->worker, server_worker, con_)
  GEARMAN_LIST_ADD(server_function->worker, server_worker, function_)

  return server_worker;
}

void gearman_server_worker_free(gearman_server_worker_st *server_worker)
{
  /* If the worker was in the middle of a job, requeue it. */
  if (server_worker->job != NULL)
    (void)gearman_server_job_queue(server_worker->job);

  GEARMAN_LIST_DEL(server_worker->con->worker, server_worker, con_)
  GEARMAN_LIST_DEL(server_worker->function->worker, server_worker, function_)

  if (server_worker->options & GEARMAN_SERVER_WORKER_ALLOCATED)
    free(server_worker);
}
