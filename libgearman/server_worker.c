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
gearman_server_worker_add(gearman_server_con_st *con, const char *function_name,
                          size_t function_name_size, uint32_t timeout)
{
  gearman_server_worker_st *worker;
  gearman_server_function_st *function;

  function= gearman_server_function_get(con->thread->server, function_name,
                                        function_name_size);
  if (function == NULL)
    return NULL;

  worker= gearman_server_worker_create(con, function, NULL);
  if (worker == NULL)
    return NULL;

  worker->timeout= timeout;

  return worker;
}

gearman_server_worker_st *
gearman_server_worker_create(gearman_server_con_st *con,
                             gearman_server_function_st *function,
                             gearman_server_worker_st *worker)
{
  gearman_server_st *server= con->thread->server;

  if (worker == NULL)
  {
    if (server->free_worker_count > 0)
    {
      worker= server->free_worker_list;
      GEARMAN_LIST_DEL(server->free_worker, worker, con_)
    }
    else
    {
      worker= malloc(sizeof(gearman_server_worker_st));
      if (worker == NULL)
      {
        GEARMAN_ERROR_SET(con->thread->gearman, "gearman_server_worker_create",
                          "malloc")
        return NULL;
      }
    }

    worker->options= GEARMAN_SERVER_WORKER_ALLOCATED;
  }
  else
    worker->options= 0;

  worker->timeout= 0;
  worker->con= con;
  GEARMAN_LIST_ADD(con->worker, worker, con_)
  worker->function= function;
  GEARMAN_LIST_ADD(function->worker, worker, function_)
  worker->job= NULL;

  return worker;
}

void gearman_server_worker_free(gearman_server_worker_st *worker)
{
  gearman_server_st *server= worker->con->thread->server;

  /* If the worker was in the middle of a job, requeue it. */
  if (worker->job != NULL)
    (void)gearman_server_job_queue(worker->job);

  GEARMAN_LIST_DEL(worker->con->worker, worker, con_)
  GEARMAN_LIST_DEL(worker->function->worker, worker, function_)

  if (worker->options & GEARMAN_SERVER_WORKER_ALLOCATED)
  {
    if (server->free_worker_count < GEARMAN_MAX_FREE_SERVER_WORKER)
      GEARMAN_LIST_ADD(server->free_worker, worker, con_)
    else
      free(worker);
  }
}
