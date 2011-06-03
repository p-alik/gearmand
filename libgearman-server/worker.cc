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

#include <libgearman-server/common.h>

static gearman_server_worker_st *
gearman_server_worker_create(gearman_server_con_st *con,
                             gearman_server_function_st *function);

/*
 * Public definitions
 */

gearman_server_worker_st *
gearman_server_worker_add(gearman_server_con_st *con, const char *function_name,
                          size_t function_name_size, uint32_t timeout)
{
  gearman_server_worker_st *worker;
  gearman_server_function_st *function;

  function= gearman_server_function_get(Server, function_name,
                                        function_name_size);
  if (function == NULL)
    return NULL;

  worker= gearman_server_worker_create(con, function);
  if (worker == NULL)
    return NULL;

  worker->timeout= timeout;

  return worker;
}

static gearman_server_worker_st *
gearman_server_worker_create(gearman_server_con_st *con, gearman_server_function_st *function)
{
  gearman_server_worker_st *worker;

  if (Server->free_worker_count > 0)
  {
    worker= Server->free_worker_list;
    GEARMAN_LIST_DEL(Server->free_worker, worker, con_)
  }
  else
  {
    worker= static_cast<gearman_server_worker_st *>(malloc(sizeof(gearman_server_worker_st)));
    if (not worker)
    {
      gearmand_merror("malloc", 0, sizeof(gearman_server_worker_st));
      return NULL;
    }
  }


  worker->job_count= 0;
  worker->timeout= 0;
  worker->con= con;
  GEARMAN_LIST_ADD(con->worker, worker, con_)
  worker->function= function;

  /* Add worker to the function list, which is a double-linked circular list. */
  if (function->worker_list == NULL)
  {
    function->worker_list= worker;
    worker->function_next= worker;
    worker->function_prev= worker;
  }
  else
  {
    worker->function_next= function->worker_list;
    worker->function_prev= function->worker_list->function_prev;
    worker->function_next->function_prev= worker;
    worker->function_prev->function_next= worker;
  }
  function->worker_count++;

  worker->job_list= NULL;

  return worker;
}

void gearman_server_worker_free(gearman_server_worker_st *worker)
{
  /* If the worker was in the middle of a job, requeue it. */
  while (worker->job_list != NULL)
  {
    gearmand_error_t ret;
    ret= gearman_server_job_queue(worker->job_list);
    if (ret != GEARMAN_SUCCESS)
    {
      gearmand_gerror("gearman_server_job_queue", ret);
    }
  }

  GEARMAN_LIST_DEL(worker->con->worker, worker, con_)

  if (worker == worker->function_next)
  {
    worker->function->worker_list= NULL;
  }
  else
  {
    worker->function_next->function_prev= worker->function_prev;
    worker->function_prev->function_next= worker->function_next;

    if (worker == worker->function->worker_list)
      worker->function->worker_list= worker->function_next;
  }
  worker->function->worker_count--;

  if (Server->free_worker_count < GEARMAN_MAX_FREE_SERVER_WORKER)
  {
    GEARMAN_LIST_ADD(Server->free_worker, worker, con_)
  }
  else
  {
    gearmand_crazy("free");
    free(worker);
  }
}
