/* Gearman server and library
 * Copyright (C) 2008 Brian Aker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <libgearman/gearman_worker.h>
#include "common.h"

gearman_worker_st *gearman_worker_create(gearman_worker_st *worker)
{
  if (worker == NULL)
  {
    worker= (gearman_worker_st *)malloc(sizeof(gearman_worker_st));

    if (!worker)
      return NULL; /*  GEARMAN_MEMORY_ALLOCATION_FAILURE */

    memset(worker, 0, sizeof(gearman_worker_st));
    worker->is_allocated= true;
  }
  else
  {
    memset(worker, 0, sizeof(gearman_worker_st));
  }

  return worker;
}

void gearman_worker_free(gearman_worker_st *worker)
{
  /* If we have anything open, lets close it now */
  gearman_server_list_free(&worker->list);

  if (worker->function_name)
    free(worker->function_name);

  if (worker->is_allocated == true)
      free(worker);
  else
    memset(worker, 0, sizeof(worker));
}

/*
  clone is the destination, while worker is the structure to clone.
  If worker is NULL the call is the same as if a gearman_worker_create() was
  called.
*/
gearman_worker_st *gearman_worker_clone(gearman_worker_st *worker)
{
  gearman_worker_st *clone;
  gearman_return rc= GEARMAN_SUCCESS;

  clone= gearman_worker_create(NULL);

  if (clone == NULL)
    return NULL;

  /* If worker was null, then we just return NULL */
  if (worker == NULL)
    return gearman_worker_create(NULL);

  rc= gearman_server_copy(&clone->list, &worker->list);

  if (rc != GEARMAN_SUCCESS)
  {
    gearman_worker_free(clone);

    return NULL;
  }

  return clone;
}

gearman_return gearman_worker_server_add(gearman_worker_st *worker,
                                         const char *hostname,
                                         uint16_t port)
{
  return gearman_server_add(&worker->list, hostname, port);
}

gearman_return gearman_server_function_register(gearman_worker_st *worker,
                                                const char *function_name,
                                                gearman_worker_function *function)
{
  gearman_return rc;
  giov_st giov[2];
  gearman_server_st *server;
  gearman_result_st result;
  uint32_t x;

  assert(function_name);
  assert(function);

  if (gearman_result_create(&result) == NULL)
    return GEARMAN_FAILURE;

  if (worker->list.number_of_hosts == 0)
    return GEARMAN_FAILURE;

  for (x= 0; x < worker->list.number_of_hosts; x++)
  {
    server= &(worker->list.hosts[x]);
    assert(server);

    if ((worker->function_name= strdup(function_name)) == NULL)
      return GEARMAN_MEMORY_ALLOCATION_FAILURE;

    worker->function= function;

    giov[0].arg= function_name;
    giov[0].arg_length= strlen(function_name);
    if (worker->time_out)
    {
      /* Create a sensible define here */
      char buffer[128];

      sprintf(buffer, "%u", worker->time_out);
      giov[1].arg= buffer;
      giov[1].arg_length= strlen(buffer);

      rc= gearman_dispatch(server,  GEARMAN_CAN_DO_TIMEOUT, giov, 1);
    }
    else
      rc= gearman_dispatch(server,  GEARMAN_CAN_DO, giov, 1);

    if (rc == GEARMAN_SUCCESS)
      rc= gearman_dispatch(server, GEARMAN_PRE_SLEEP, NULL, 0);
  }

  gearman_result_free(&result);

  return GEARMAN_SUCCESS;
}

gearman_return gearman_server_work(gearman_worker_st *worker)
{
  gearman_return rc;
  giov_st giov[2];
  gearman_server_st *server;
  gearman_result_st result;
  uint32_t x;

  if (gearman_result_create(&result) == NULL)
    return GEARMAN_FAILURE;

  if (worker->list.number_of_hosts == 0)
    return GEARMAN_FAILURE;

  while (1)
  {
    for (x= 0; x < worker->list.number_of_hosts; x++)
    {
      server= &(worker->list.hosts[x]);
      assert(server);

      rc= gearman_dispatch(server, GEARMAN_GRAB_JOB, NULL, true); 
      WATCHPOINT_ERROR(rc);

      rc= gearman_response(server, &result);

      switch (result.action)
      {
      case GEARMAN_JOB_ASSIGN: /* We do a job, and then exit */
        {
          uint8_t *worker_result;
          ssize_t worker_result_length;

          worker_result= (*(worker->function))(worker, 
                                               result.value->value, result.value->length,
                                               &worker_result_length,
                                               &rc);

          giov[0].arg= result.handle->value;
          giov[0].arg_length= result.handle->length;
          giov[1].arg= worker_result;
          giov[1].arg_length= worker_result_length;

          rc= gearman_dispatch(server, GEARMAN_GRAB_JOB, giov, true); 

          if (worker_result)
            free(worker_result);

          goto finished_a_job;
        }
      case GEARMAN_NO_JOB:
        continue;
      case GEARMAN_NOOP: /* Retry server */
        x--;
        continue;
      default:
        WATCHPOINT_ACTION(result.action);
        assert(1);
      }
    }
  }
finished_a_job:

  gearman_result_free(&result);

  return GEARMAN_SUCCESS;
}
